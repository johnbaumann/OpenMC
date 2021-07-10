#include "sio_memory_card.h"

#include "flashdata.h"
#include "sio.h"
#include "spi.h"

#include <esp_log.h>
#include <esp_attr.h>

namespace esp_sio_dev
{
  namespace sio
  {
    namespace memory_card
    {

      uint8_t MemCardRAM[131072] = {};

      uint8_t FLAG = Flags::kDirectoryUnread;

      uint8_t Cur_Cmnd;
      uint16_t Cmnd_Ticks;
      uint16_t Sector;
      uint8_t Sector_Offset;
      uint8_t Checksum_In;
      uint8_t Checksum_Out;
      //bool SendAck = true;
      bool UncommitedWrite = false;

      uint8_t DataBuffer[128]; //128 on 328P

      uint8_t GameID[256];
      uint8_t GameID_Length;

      void Commit()
      {
        if (UncommitedWrite)
        {
          // Write buffer to memory page
          //optiboot_writePage(FlashData, DataBuffer, Sector + 1);
          for (int i = 0; i < 128; i++)
          {
            MemCardRAM[(Sector * 128) + i] = DataBuffer[i];
          }

          // Directory structure was updated, reset directory status
          if (Sector == 0x0000)
            FLAG = Flags::kDirectoryUnread;

          // Clear buffer status before return
          UncommitedWrite = false;
        }

        return;
      }

      void GoIdle()
      {
        Cur_Cmnd = Commands::kNone;
        Cmnd_Ticks = 0;
        //SendAck = true;
        Sector_Offset = 0;
      }

      uint8_t IRAM_ATTR ProcessEvents(uint8_t DataIn)
      {
        uint8_t DataOut;
        bool cmdRouted = false;

        //Loop until command is properly routed
        while (!cmdRouted)
        {
          switch (Cur_Cmnd)
          {
            // No command yet
          case Commands::kNone:
            // Store incoming byte as command
            Cur_Cmnd = DataIn;
            // Store FLAG byte in outbound variable
            DataOut = FLAG;
            // Safe to exit interpret loop
            cmdRouted = true;
            break;

          // Last command was MC Access, expecting
          case Commands::kAccess:
            switch (DataIn)
            {
            case Commands::kRead:
            case Commands::kWrite:
            case Commands::kGetID:
            case Commands::kGameID:
            case Commands::kPing:
              Cur_Cmnd = DataIn;
              // Re-evaluate command
              // cmdRouted = false;
              break;

            default:
              Cur_Cmnd = Commands::kError;
              // Re-evaluate command
              // cmdRouted = false;
            }
            break;

          case Commands::kRead:
            DataOut = TickReadCommand(DataIn);
            cmdRouted = true;
            break;

          case Commands::kWrite:
            DataOut = TickWriteCommand(DataIn);
            cmdRouted = true;
            break;

          // https://gitlab.com/chriz2600/ps1-game-id-transmission
          // PS1 Game ID transmisson (MemCard PRO / PS1Digital)
          case Commands::kGameID:
            DataOut = TickGameIDCommand(DataIn);
            cmdRouted = true;
            break;

          case Commands::kPing:
            DataOut = TickPingCommand(DataIn);
            cmdRouted = true;
            break;

          case Commands::kGetID: // Un-implemented, need data capture
          case Commands::kError: // Unexpected/Unsupported command
          default:
            DataOut = Responses::kIdleHighZ;
            GoIdle();
            cmdRouted = true;
            break;
          }
        }

        return DataOut;
      }

      uint8_t IRAM_ATTR TickReadCommand(uint8_t &DataIn)
      {
        uint8_t DataOut = 0xFF;

        //SendAck = true; // Default true;

        switch (Cmnd_Ticks)
        {
          //Data is sent and received simultaneously,
          //so the data we send isn't received by the system
          //until the next bytes are exchanged. In this way,
          //you have to basically respond one byte earlier than
          //actually occurs between Slave and Master.
          //Offset "Send" bytes noted from nocash's psx specs.

        case 0: //52h
          DataOut = Responses::kID1;
          break;

        case 1: //00h
          DataOut = Responses::kID2;
          break;

        case 2: //00h
          DataOut = Responses::kDummy;
          break;

        case 3: //MSB
          //Store upper 8 bits of sector
          Sector = (DataIn << 8);
          //Reply with (pre)
          DataOut = DataIn;
          break;

        case 4: //LSB
          //Store lower 8 bits of sector
          Sector |= DataIn;
          DataOut = Responses::kCommandAcknowledge1;
          break;

        case 5: //00h
          DataOut = Responses::kCommandAcknowledge2;
          break;

        case 6: //00h
          //Confirm MSB
          DataOut = Sector >> 8;
          break;

        case 7: //00h
          //Confirm LSB
          DataOut = (Sector & 0xFF);
          Checksum_Out = highByte(Sector) ^ lowByte(Sector);
          //ets_printf("Sector = 0x%04X\n", Sector);
          break;

          // Cases 8 through 135 overloaded to default operator below

        case 136:
          DataOut = Checksum_Out;
          break;

        case 137:
          DataOut = Responses::kGoodReadWrite;
          //SendAck = false;
          break;

        default:
          if (Cmnd_Ticks >= 8 && Cmnd_Ticks <= 135) //Stay here for 128 bytes
          {
            if (Sector >= 1024)
            {
              DataOut = 0x00;
            }
            else
            {
              DataOut = MemCardRAM[(Sector * (uint16_t)128) + Sector_Offset];
            }

            Checksum_Out ^= DataOut;
            Sector_Offset++;
          }
          else
          {
            // Send this till the spooky extra bytes go away
            DataOut = Responses::kCommandAcknowledge1;
          }
          break;
        }

        Cmnd_Ticks++;

        return DataOut;
      }

      uint8_t TickWriteCommand(uint8_t &DataIn)
      {
        uint8_t DataOut = 0xFF;

        //SendAck = true; // Default true;

        switch (Cmnd_Ticks)
        {
          // Data is sent and received simultaneously,
          // so the data we send isn't received by the system
          // until the next bytes are exchanged. In this way,
          // you have to basically respond one byte earlier than
          // actually occurs between Slave and Master.
          // Offset "Send" bytes noted from nocash's psx specs.

        case 0: // 52h
          DataOut = Responses::kID1;
          break;

        case 1: // 00h
          DataOut = Responses::kID2;
          break;

        case 2: // 00h
          DataOut = Responses::kDummy;
          break;

        case 3: // MSB
          // Store upper 8 bits of sector
          Sector = (DataIn << 8);
          // Reply with (pre)
          DataOut = DataIn;
          break;

        case 4: // LSB
          // Store lower 8 bits of sector
          Sector |= DataIn;
          Checksum_Out = (Sector >> 8) ^ (Sector & 0xFF);
          DataOut = DataIn;

          break;

        default:
          if (Cmnd_Ticks >= 5 && Cmnd_Ticks <= 132)
          {
            // Store data
            DataBuffer[Cmnd_Ticks - 5] = DataIn;
            // Calculate checksum
            Checksum_Out ^= DataIn;
            // Reply with (pre)
            DataOut = DataIn;
          }
          else if (Cmnd_Ticks == 133) // CHK
          {
            Checksum_In = DataIn;
            DataOut = Responses::kCommandAcknowledge1;
          }
          else if (Cmnd_Ticks == 134) // 00h
          {
            DataOut = Responses::kCommandAcknowledge2;
          }
          else if (Cmnd_Ticks == 135) // 00h
          {
            if (Sector >= 1024)
            {
              DataOut = Responses::kBadSector;
            }
            else if (Checksum_In == Checksum_Out)
            {
              FLAG = Flags::kDirectoryRead;
              DataOut = Responses::kGoodReadWrite;
              // If the incoming sector is within our storage, store it
              if (Sector < 1024)
              {
                UncommitedWrite = true;
              }
            }
            else
            {
              DataOut = Responses::kBadChecksum;
            }

            GoIdle();
          }
          else
          {
            GoIdle();
          }

          break;
        }

        Cmnd_Ticks++;

        return DataOut;
      }

      uint8_t TickGameIDCommand(uint8_t &DataIn)
      {
        uint8_t DataOut = 0xFF;

        switch (Cmnd_Ticks)
        {
        case 0: // 21h GameID command start
        case 1: // Reserved
          DataOut = Responses::kReserved;
          break;

        case 2: // Length of the game id transmitted ( strlen )
          DataOut = Responses::kReserved;
          GameID_Length = DataIn;
          break;

        default:
          if (Cmnd_Ticks >= 3 && (Cmnd_Ticks - 3) <= GameID_Length)
          {
            GameID[Cmnd_Ticks - 3] = DataIn;
            DataOut = Responses::kReserved;
          }
          else
          {
            DataOut = Responses::kCommandAcknowledge1;
          }
        }

        Cmnd_Ticks++;

        return DataOut;
      }

      uint8_t TickPingCommand(uint8_t &DataIn)
      {
        uint8_t DataOut = 0xFF;

        switch (Cmnd_Ticks)
        {

        case 0:             // 20h Ping Command
        case 1:             // 00h Reserved
        case 2:             // 00h Reserved
          DataOut = kDummy; // 0x00 / Reserved
          break;

        case 3: // Card Present
          DataOut = Responses::kReserved;
          break;

        case 4: // Termination signal
          DataOut = Responses::kTerminationSignal;
          break;

        default:
          DataOut = Responses::kCommandAcknowledge1;
        }

        Cmnd_Ticks++;

        return DataOut;
      }
    } // namespace memory_card
  } // namespace sio
} // namespace esp_sio_dev