#include "sio_memory_card.h"

#include "storage/storage.h"
#include "logging.h"
#include "playstation/sio.h"
#include "playstation/spi.h"

#include <esp_log.h>
#include <esp_attr.h>
#include <esp32/rom/ets_sys.h>

#include <stdio.h>

//#define MC_DEBUG

namespace openmc
{
  namespace sio
  {
    namespace memory_card
    {
#ifdef MC_DEBUG
      static char debug_string[] = "MC event %2X\n";
#endif
      uint8_t DRAM_ATTR memory_card_ram[131072] = {};

      uint8_t DRAM_ATTR flag = Flags::kDirectoryUnread;

      static uint8_t DRAM_ATTR current_command;
      static uint16_t DRAM_ATTR command_ticks;

      uint16_t DRAM_ATTR sector;
      static uint8_t DRAM_ATTR sector_offset;

      static uint8_t DRAM_ATTR checksum_in;
      static uint8_t DRAM_ATTR checksum_out;

      static bool DRAM_ATTR uncommited_soft_write = false;
      bool DRAM_ATTR committed_to_storage = true;
      uint64_t volatile DRAM_ATTR last_tick;
      uint64_t volatile DRAM_ATTR last_write_tick;

      static uint8_t DRAM_ATTR data_buffer[128];

      uint8_t DRAM_ATTR game_id[256];
      uint8_t DRAM_ATTR game_id_length;

      void IRAM_ATTR Commit()
      {
        bool sector_changed = false;
        uint32_t sector_offset = (sector * 128);

        if (uncommited_soft_write)
        {
          // Write buffer to memory page
          for (int i = 0; i < 128; i++)
          {
            if (memory_card_ram[sector_offset + i] != data_buffer[i])
            {
              sector_changed = true;
              memory_card_ram[sector_offset + i] = data_buffer[i];
            }
          }

          // Directory structure was updated, reset directory status
          if (sector == 0x0000)
          {
            flag = Flags::kDirectoryUnread;
          }

          // Clear (soft)buffer status before return
          uncommited_soft_write = false;

          if (sector_changed)
          {
            committed_to_storage = false;
          }
          last_write_tick = sio::event_ticks;
        }

        return;
      }

      void IRAM_ATTR GoIdle()
      {
        current_command = Commands::kNone;
        command_ticks = 0;
        sector_offset = 0;
      }

      uint8_t IRAM_ATTR ProcessEvents(uint8_t data_in)
      {
        uint8_t data_out;
        bool command_routed = false;

        last_tick = sio::event_ticks;

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
#ifdef MC_DEBUG
            ets_printf(debug_string, data_in);
#endif
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
#ifdef MC_DEBUG
              ets_printf(debug_string, data_in);
#endif
              break;

            default:
              data_out = Responses::kIdleHighZ;
              GoIdle();
              // Re-evaluate command
              // command_routed = false;
#ifdef MC_DEBUG
              ets_printf(debug_string, data_in);
#endif
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
          break;

          // Cases 8 through 135 overloaded to default operator below

        case 136:
          data_out = checksum_out;
          break;

        case 137:
          data_out = Responses::kGoodReadWrite;
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

        case 133: // CHK
          checksum_in = data_in;
          data_out = Responses::kCommandAcknowledge1;
          break;

        case 134: // 00h
          data_out = Responses::kCommandAcknowledge2;
          break;

        case 135: // 00h
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
              uncommited_soft_write = true;
            }
          }
          else
          {
            data_out = Responses::kBadChecksum;
          }
          GoIdle();
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

  } // namespace sio
} // namespace openmc