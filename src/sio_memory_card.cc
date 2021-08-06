#include "sio_memory_card.h"

#include "esp_file_helper.h"
#include "esp_logging.h"
#include "sio.h"
#include "spi.h"

#include <esp_log.h>
#include <esp_attr.h>
#include <esp32/rom/ets_sys.h>

#include <stdio.h>

namespace esp_sio_dev
{
  namespace sio
  {
    namespace memory_card
    {

      uint8_t DRAM_ATTR memory_card_ram[131072] = {};

      uint8_t flag = Flags::kDirectoryUnread;

      uint8_t current_command;
      uint16_t command_ticks;
      uint16_t sector;
      uint8_t sector_offset;
      uint8_t checksum_in;
      uint8_t checksum_out;
      //bool SendAck = true;
      bool uncommited_write = false;

      uint8_t DRAM_ATTR data_buffer[128]; //128 on 328P

      uint8_t game_id[256];
      uint8_t game_id_length;

      void IRAM_ATTR Commit()
      {
        if (uncommited_write)
        {
          // Write buffer to memory page
          for (int i = 0; i < 128; i++)
          {
            memory_card_ram[(sector * 128) + i] = data_buffer[i];
          }

          // Directory structure was updated, reset directory status
          if (sector == 0x0000)
            flag = Flags::kDirectoryUnread;

          //ets_printf("Soft commit sector %d\n", sector);

          mc_sector_uncommitted[sector] = true;
          mc_hard_committed = false;

          //myparam.dest_mc_ram = memory_card_ram;
          //myparam.dest_sector = sector;
          //Create_Write_Task((void *)&myparam);

          // Clear (soft)buffer status before return
          uncommited_write = false;
        }

        return;
      }

      void IRAM_ATTR GoIdle()
      {
        current_command = Commands::kNone;
        command_ticks = 0;
        //SendAck = true;
        sector_offset = 0;
      }

      uint8_t IRAM_ATTR ProcessEvents(uint8_t data_in)
      {
        uint8_t data_out;
        bool command_routed = false;

        //Loop until command is properly routed
        while (!command_routed)
        {
          switch (current_command)
          {
            // No command yet
          case Commands::kNone:
            // Store incoming byte as command
            current_command = data_in;
            // Store flag byte in outbound variable
            data_out = flag;
            // Safe to exit interpret loop
            command_routed = true;
            break;

          // Last command was MC Access, expecting
          case Commands::kAccess:
            switch (data_in)
            {
            case Commands::kRead:
            case Commands::kWrite:
            case Commands::kGetID:
            case Commands::kGameID:
            case Commands::kPing:
              current_command = data_in;
              // Re-evaluate command
              // command_routed = false;
              break;

            default:
              data_out = Responses::kIdleHighZ;
              GoIdle();
              //ets_printf("Unexpected MC command %x\n", data_in);
              //current_command = Commands::kError;
              // Re-evaluate command
              // command_routed = false;
            }
            break;

          case Commands::kRead:
            data_out = TickReadCommand(data_in);
            command_routed = true;
            break;

          case Commands::kWrite:
            data_out = TickWriteCommand(data_in);
            command_routed = true;
            break;

          // https://gitlab.com/chriz2600/ps1-game-id-transmission
          // PS1 Game ID transmisson (MemCard PRO / PS1Digital)
          case Commands::kGameID:
            data_out = TickGameIDCommand(data_in);
            command_routed = true;
            break;

          case Commands::kPing:
            data_out = TickPingCommand(data_in);
            command_routed = true;
            break;

          case Commands::kGetID: // Un-implemented, need data capture
          case Commands::kError: // Unexpected/Unsupported command
          default:
            data_out = Responses::kIdleHighZ;
            GoIdle();
            command_routed = true;
            break;
          }
        }

        return data_out;
      }

      uint8_t IRAM_ATTR TickReadCommand(uint8_t &data_in)
      {
        uint8_t data_out = 0xFF;

        //SendAck = true; // Default true;

        switch (command_ticks)
        {
          //Data is sent and received simultaneously,
          //so the data we send isn't received by the system
          //until the next bytes are exchanged. In this way,
          //you have to basically respond one byte earlier than
          //actually occurs between Slave and Master.
          //Offset "Send" bytes noted from nocash's psx specs.

        case 0: //52h
          data_out = Responses::kID1;
          break;

        case 1: //00h
          data_out = Responses::kID2;
          break;

        case 2: //00h
          data_out = Responses::kDummy;
          break;

        case 3: //MSB
          //Store upper 8 bits of sector
          sector = (data_in << 8);
          //Reply with (pre)
          data_out = data_in;
          break;

        case 4: //LSB
          //Store lower 8 bits of sector
          sector |= data_in;
          data_out = Responses::kCommandAcknowledge1;
          break;

        case 5: //00h
          data_out = Responses::kCommandAcknowledge2;
          break;

        case 6: //00h
          //Confirm MSB
          data_out = sector >> 8;
          break;

        case 7: //00h
          //Confirm LSB
          data_out = (sector & 0xFF);
          checksum_out = highByte(sector) ^ lowByte(sector);
          //ets_printf("Sector = 0x%04X\n", Sector);
          break;

          // Cases 8 through 135 overloaded to default operator below

        case 136:
          data_out = checksum_out;
          break;

        case 137:
          data_out = Responses::kGoodReadWrite;
          //SendAck = false;
          break;

        default:
          if (command_ticks >= 8 && command_ticks <= 135) //Stay here for 128 bytes
          {
            if (sector >= 1024)
            {
              data_out = 0x00;
            }
            else
            {
              data_out = memory_card_ram[(sector * (uint16_t)128) + sector_offset];
            }

            checksum_out ^= data_out;
            sector_offset++;
          }
          else
          {
            // Send this till the spooky extra bytes go away
            data_out = Responses::kCommandAcknowledge1;
          }
          break;
        }

        command_ticks++;

        return data_out;
      }

      uint8_t IRAM_ATTR TickWriteCommand(uint8_t &data_in)
      {
        uint8_t data_out = 0xFF;

        //SendAck = true; // Default true;

        switch (command_ticks)
        {
          // Data is sent and received simultaneously,
          // so the data we send isn't received by the system
          // until the next bytes are exchanged. In this way,
          // you have to basically respond one byte earlier than
          // actually occurs between Slave and Master.
          // Offset "Send" bytes noted from nocash's psx specs.

        case 0: // 52h
          data_out = Responses::kID1;
          break;

        case 1: // 00h
          data_out = Responses::kID2;
          break;

        case 2: // 00h
          data_out = Responses::kDummy;
          break;

        case 3: // MSB
          // Store upper 8 bits of sector
          sector = (data_in << 8);
          // Reply with (pre)
          data_out = data_in;
          break;

        case 4: // LSB
          // Store lower 8 bits of sector
          sector |= data_in;
          checksum_out = (sector >> 8) ^ (sector & 0xFF);
          data_out = data_in;

          break;

        default:
          if (command_ticks >= 5 && command_ticks <= 132)
          {
            // Store data
            data_buffer[command_ticks - 5] = data_in;
            // Calculate checksum
            checksum_out ^= data_in;
            // Reply with (pre)
            data_out = data_in;
          }
          else if (command_ticks == 133) // CHK
          {
            checksum_in = data_in;
            data_out = Responses::kCommandAcknowledge1;
          }
          else if (command_ticks == 134) // 00h
          {
            data_out = Responses::kCommandAcknowledge2;
          }
          else if (command_ticks == 135) // 00h
          {
            if (sector >= 1024)
            {
              data_out = Responses::kBadSector;
            }
            else if (checksum_in == checksum_out)
            {
              flag = Flags::kDirectoryRead;
              data_out = Responses::kGoodReadWrite;
              // If the incoming sector is within our storage, store it
              if (sector < 1024)
              {
                uncommited_write = true;
              }
            }
            else
            {
              data_out = Responses::kBadChecksum;
            }

            GoIdle();
          }
          else
          {
            current_command = PS1_SIOCommands::Ignore;
            spi::Disable();
          }

          break;
        }

        command_ticks++;

        return data_out;
      }

      uint8_t IRAM_ATTR TickGameIDCommand(uint8_t &data_in)
      {
        uint8_t data_out = 0xFF;

        switch (command_ticks)
        {
        case 0: // 21h game_id command start
        case 1: // Reserved
          data_out = Responses::kReserved;
          break;

        case 2: // Length of the game id transmitted ( strlen )
          data_out = Responses::kReserved;
          game_id_length = data_in;
          break;

        default:
          if (command_ticks >= 3 && (command_ticks - 3) <= game_id_length)
          {
            game_id[command_ticks - 3] = data_in;
            data_out = Responses::kReserved;
          }
          else
          {
            data_out = Responses::kCommandAcknowledge1;
          }
        }

        command_ticks++;

        return data_out;
      }

      uint8_t IRAM_ATTR TickPingCommand(uint8_t &data_in)
      {
        uint8_t data_out = 0xFF;

        switch (command_ticks)
        {

        case 0:              // 20h Ping Command
        case 1:              // 00h Reserved
        case 2:              // 00h Reserved
          data_out = kDummy; // 0x00 / Reserved
          break;

        case 3: // Card Present
          data_out = Responses::kReserved;
          break;

        case 4: // Termination signal
          data_out = Responses::kTerminationSignal;
          break;

        default:
          data_out = Responses::kCommandAcknowledge1;
        }

        command_ticks++;

        return data_out;
      }
    } // namespace memory_card
  }   // namespace sio
} // namespace esp_sio_dev