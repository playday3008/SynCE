// $Id$
#include "rra.h"
#include <synce.h>
#include <synce_log.h>
#include <rapi.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "dbstream.h"
#include "contact.h"

#define RRAC_PORT 5678

#if 0
enum Command
{
UNKNOWN_COMMAND,
COMMAND_67,
COMMAND_6F_ANY,
COMMAND_6F_6,
COMMAND_6F_C1,
COMMAND_70_2_1,
COMMAND_70_2_2
};

enum Action
{
UNKNOWN_ACTION,
MODIFY, 
DELETE
};

Command last_sent;
Action action;
uint32_t current_type;
uint32_t current_id;
bool expect_data = false;

#endif

typedef struct
{
uint32_t offset_00;
WCHAR name1[100];
WCHAR name2[80];
uint32_t type_id;
uint32_t object_count;
uint32_t total_size;
FILETIME filetime;
} ObjectTypeRecord;

/* Dump a block of data for debugging purposes */
static void
dump(const char *desc, void* data, size_t len)
{
uint8_t* buf = (uint8_t*)data;
size_t i, j;
char hex[8 * 3 + 1];
char chr[8 + 1];

printf("%s (%d bytes):\n", desc, len);
for (i = 0; i < len + 7; i += 8) {
		for (j = 0; j < 8; j++) 
	if (j + i >= len) {
		hex[3*j+0] = ' ';
		hex[3*j+1] = ' ';
		hex[3*j+2] = ' ';
		chr[j] = ' ';
	} else {
		uint8_t c = buf[j + i];
		const char *hexchr = "0123456789abcdef";
		hex[3*j+0] = hexchr[(c >> 4) & 0xf];
		hex[3*j+1] = hexchr[c & 0xf];
		hex[3*j+2] = ' ';
		if (c > ' ' && c <= '~')
			chr[j] = c;
		else
			chr[j] = '.';
	}
		hex[8*3] = '\0';
		chr[8] = '\0';
		printf("  %04x: %s %s\n", i, hex, chr);
}
}

#if 0

void send_command_67(
	Socket* client, 
	uint32_t type, 
	uint32_t count, 
	uint32_t* oids)
{
struct
{
	uint16_t command;
	uint16_t size;
	uint32_t unknown;
	uint32_t type;
	uint32_t count;
} header;

if (count)
{
	synce_trace("Sending package!");

	header.command = 0x0067;
	header.size    = sizeof(header) - 4 + count * sizeof(uint32_t);
	header.unknown = 0;
	header.type    = type;
	header.count   = count;
	dump("command 67 header", &header, sizeof(header));
	client->Write(&header, sizeof(header));
	client->Write(oids, count * sizeof(uint32_t));

	expect_data = true;
	last_sent = COMMAND_67;
}
}


void send_command_6f(Socket* client, uint32_t subcommand)
{
struct
{
	uint16_t command;
	uint16_t size;
	uint32_t subcommand;
} package;

synce_trace("Subcommand: %08x", subcommand);

package.command = 0x006f;
package.size = sizeof(package) - 4;
package.subcommand = subcommand;
client->Write(&package, sizeof(package));
last_sent = COMMAND_6F_ANY;
}

void send_command_6f_6(Socket* client)
{
send_command_6f(client, 6);
last_sent = COMMAND_6F_6;
}

void send_command_6f_c1(Socket* client)
{
send_command_6f(client, 0xc1);
last_sent = COMMAND_6F_C1;
}

void send_command_70_2_1(Socket* client)
{
struct
{
	uint16_t command;
	uint16_t size;
	uint32_t size2;
	uint32_t unknown1;
	uint32_t subcommand;
	uint8_t empty1[0xc8];
	uint32_t subsubcommand;
	uint32_t unknown2;
	uint8_t empty2[0x18];
} package;

package.command = 0x70;
package.size = sizeof(package) - 4;
package.size2 = package.size - 4;
package.unknown1 = 0xf0000001;
package.subcommand = 2;
memset(package.empty1, 0, sizeof(package.empty1));
package.subsubcommand = 1;
package.unknown2 = 0x80000003;
memset(package.empty2, 0, sizeof(package.empty2));

dump("command 70,2,1", &package, sizeof(package));

client->Write(&package, sizeof(package));
last_sent = COMMAND_70_2_1;
}

void send_command_70_2_2(Socket* client)
{
struct
{
	uint16_t command;
	uint16_t size;
	uint32_t size2;
	uint32_t unknown1;
	uint32_t subcommand;
	uint8_t empty1[0xc8];
	uint32_t subsubcommand;
	uint32_t unknown2;
	uint8_t empty2[0x18];
} package;

package.command = 0x70;
package.size = sizeof(package) - 4;
package.size2 = package.size - 4;
package.unknown1 = 0xf0000001;
package.subcommand = 2;
memset(package.empty1, 0, sizeof(package.empty1));
package.subsubcommand = 2;
package.unknown2 = 0;
memset(package.empty2, 0, sizeof(package.empty2));

dump("command 70,2,2", &package, sizeof(package));

client->Write(&package, sizeof(package));
last_sent = COMMAND_70_2_2;
}

void send_command_70_3(Socket* client, uint32_t count, uint32_t* oids)
{
struct
{
	uint16_t command;
	uint16_t size;
	uint32_t size2;
	uint32_t stuff[6];
	uint32_t count;
} package;

package.command = 0x70;
package.size = sizeof(package) - 4 + count * sizeof(uint32_t);
package.size2 = package.size - 4;
package.stuff[0] = 0xf0000001;
package.stuff[1] = 3;
package.stuff[2] = 2;
package.stuff[3] = 0;
package.stuff[4] = 0;
package.stuff[5] = 0;
package.count = count;

dump("Package", &package, sizeof(package));
client->Write(&package, sizeof(package));
if (count)
	client->Write(oids, count * sizeof(uint32_t));
}

void handle_command_65(Socket* client)
{
uint16_t size = client->Read16();
unsigned char* buffer = client->Read(size);
dump("command 65", buffer, size);
delete[] buffer;
}

void handle_command_69_subcommand_2(Socket* client, unsigned size)
{
struct package_type
{
	uint32_t stuff[3];
	uint32_t partner_index;
	uint32_t partner_id[2];
} *package;

synce_warning_unless(size == 0x18, "Unexpected size: %04x", size);

package = (package_type*)client->Read(size);

synce_warning_unless(0xffffffff == package->stuff[0], 
		"Unexpected value for stuff[0]: %08x", package->stuff[0]);
	synce_warning_unless(3 == package->stuff[1], 
		"Unexpected value for stuff[1]: %08x", package->stuff[1]);
	synce_warning_unless(0xc == package->stuff[2], 
		"Unexpected value for stuff[2]: %08x", package->stuff[2]);

synce_trace("Partner info: index = %i, ID 1 = %08x, ID 2 = %08x",
		package->partner_index, package->partner_id[0], package->partner_id[1]);

delete package;
}

void handle_command_69_subcommand_0_or_4_or_6(Socket* client, unsigned subcommand, unsigned size)
{
struct package_type
{
	uint32_t oid;
	uint32_t some_count;
	uint32_t size;
} *package;

uint8_t* buffer = client->Read(size);
package = (package_type*)buffer;

int total_count = package->size / 4;

synce_trace("subcommand=%08x, oid=0x%x, modified=%i, deleted=%i, size=0x%x",
		subcommand, package->oid, package->some_count, total_count -
		package->some_count, package->size);

uint32_t* ids = (uint32_t*)(buffer + sizeof(package_type));

switch (subcommand)
{
	case 0:
		synce_warning_unless(
				package->size == 4, 
				"Unexpected package size: %x",
				package->size);

		action = package->some_count ? MODIFY : DELETE;
		current_type = package->oid;
		current_id = ids[0];
		send_command_70_2_1(client);
		break;

	case 0x06000000:
		send_command_6f_6(client);
		break;
}

//	send_command_67(client, package->oid, package->some_count, ids);


delete[] buffer;

#if 0
unsigned char* buffer = client->Read(size);
dump("command 69, subcommand 4 or 6", buffer, size);
delete[] buffer;
#endif
}

void handle_command_69_unknown_subcommand(Socket* client, unsigned size)
{
unsigned char* buffer = client->Read(size);
dump("command 69, unknown subcommand", buffer, size);
delete[] buffer;
}

void handle_command_69(Socket* client)
{
uint16_t size = client->Read16();
synce_trace("Size: %04x", size);

uint32_t subcommand = client->Read32();
size -= 4;

synce_trace("Subcommand: %08x", subcommand);

switch (subcommand)
{
	case 0x02000000: handle_command_69_subcommand_2(client, size); break;
	case 0x00000000: handle_command_69_subcommand_0_or_4_or_6(client, subcommand, size); break;
	case 0x04000000: handle_command_69_subcommand_0_or_4_or_6(client, subcommand, size); break;
	case 0x06000000: handle_command_69_subcommand_0_or_4_or_6(client, subcommand, size); break;
	default:   			 handle_command_69_unknown_subcommand(client, size); break;
}
}

void dump_objtypeinfo(ObjectTypeRecord* typeinfo)
{
char* name1 = wstr_to_ascii(typeinfo->name1);
char* name2 = wstr_to_ascii(typeinfo->name2);

synce_trace("Type: \"%s\", \"%s\", oid=0x%x, count=%i, size=%i", name1, name2,
		typeinfo->oid, typeinfo->object_count, typeinfo->total_size);

wstr_free_string(name1);
wstr_free_string(name2);
}

void handle_command_6f_reply(Socket* client, unsigned size)
{
uint8_t* buffer = client->Read(size);

dump("reply to command 6f", buffer, 0x20);

switch (last_sent)
{
	case COMMAND_6F_6:
		dump("reply to command 6f,6", buffer, size);
		break;

	case COMMAND_6F_C1:
		{
			uint32_t count = *(uint32_t*)(buffer + 0x1c);

			ObjectTypeRecord* typeinfo = (ObjectTypeRecord*)(buffer + 0x20);

			for (uint32_t i = 0; i < count; i++)
				dump_objtypeinfo(typeinfo+i);

			synce_warning_unless(0x20 == (size - (count * sizeof(ObjectTypeRecord))),
					"Unexpected size of command");

			uint32_t* oids = new uint32_t[count - 1];

			int j = 0;
			for (unsigned i = 0; i < count; i++)
			{
				if (typeinfo[i].oid != 0x2712)
					oids[j++] = typeinfo[i].oid;
			}

			send_command_70_3(client, count - 1, oids);

			delete[] oids;
		}
		break;
}

delete[] buffer;
}

void handle_command_70_reply(Socket* client, unsigned size)
{
unsigned char* buffer = client->Read(size);
dump("reply to command 70", buffer, size);
delete[] buffer;

if (COMMAND_70_2_1 == last_sent)
{
	switch (action)
	{
		case MODIFY:
			send_command_67(client, current_type, 1, &current_id);
			break;
		
		case DELETE:
			send_command_70_2_2(client);
			send_command_6f_6(client);
			break;
	}
}
}

void handle_unknown_reply(Socket* client, unsigned size)
{
unsigned char* buffer = client->Read(size);
dump("reply to unknown command", buffer, size);
delete[] buffer;
}

void handle_command_6c(Socket* client)
{
uint16_t size = client->Read16();
synce_trace("Size: %04x", size);

uint32_t reply_command = client->Read32();
size -= 4;

synce_trace("Reply to: %08x", reply_command);

switch (reply_command)
{
	case 0x6f:  handle_command_6f_reply(client, size);  break;
	case 0x70:  handle_command_70_reply(client, size);  break;
	default:    handle_unknown_reply(client, size);     break;
}
}

void handle_unknown_command(Socket* client)
{
uint16_t size = client->Read16();
unsigned char* buffer = client->Read(size);
dump("reply to unknown command", buffer, size);
delete[] buffer;
}

void read_command(Socket* client)
{
uint16_t command = client->Read16();

synce_trace("Command: %04x", command);

switch (command)
{
	case 0x0065:  handle_command_65(client);       break;
	case 0x0069:  handle_command_69(client);       break;
	case 0x006c:  handle_command_6c(client);       break;
	default:      handle_unknown_command(client);  break;
}
}

void read_data(Socket* client)
{
struct package_header_type 
{
	uint32_t id;
	uint32_t type;
	uint32_t empty;
} *package_header;

package_header = (package_header_type*)client->Read(sizeof(package_header_type));

synce_trace(
		"id=%08x, type=%08x, empty=%08x",
		package_header->id,
		package_header->type,
		package_header->empty);

if (package_header->id == 0xffffffff)
{
	expect_data = false;
}
else
{
	uint16_t size = client->Read16();
	uint8_t* buffer = client->Read(size);

	dump("data buffer", buffer, size);

	delete[] buffer;
}

delete package_header;
}

void handle_client(Socket* client)
{
synce_trace("********** Got client **********");

send_command_6f_c1(client);

for (;;)
{
	SocketEvents events = EVENT_READ;
	client->Wait(-1, events);

	if (events & EVENT_READ)
	{
		if (expect_data)
		{
			read_data(client);
			}
			else
			{
				read_command(client);
			}
		}
	}
}

#endif

class Command;
class AggregatedCommand;
class AnyCommand;
class Reply;
class Command_65;
class Command_67;
class Command_6f;
class Command_70_2;
class Command_70_3;
class ObjectIdList;

class ReplicationAgent
{
	public:
		void Start();
		void EventLoop();

		void OnCommand (AggregatedCommand& command) {}
		void OnCommand (AnyCommand& command) {}
		void OnCommand (Reply& command) {}
		void OnCommand (Command_65& command) {}
		void OnCommand (Command_67& command) {}
		void OnCommand (Command_6f& command) {}
		void OnCommand (Command_70_2& command) {}
		void OnCommand (Command_70_3& command) {}
		void OnCommand (ObjectIdList& objectIdList);

	private:
		AnyCommand* ReadCommand();

		void DispatchCommand(AnyCommand* command);
		
		void WriteCommand(Command& command);

	private:
		Socket mServer;
		Socket* mpCmdChannel;
		Socket* mpDataChannel;
};


/*
 * Abstract base class for commands
 */
class Command/*{{{*/
{
	public:
		/** Command code in host byte order */
		virtual uint16_t  Code() { return mCode; }
		
		/** Command data size in host byte order */
		virtual uint16_t  Size() = 0;
		
		/** Command data in little endian byte order */
		virtual uint8_t*  Data() = 0;

		virtual void Dispatch(ReplicationAgent& ra)  = 0;

	protected:
		Command(uint16_t code)
			: mCode(code)
		{}

	private:
		uint16_t  mCode;
};/*}}}*/

class AnyCommand : public Command/*{{{*/
{
	public:
		AnyCommand(uint16_t code, uint16_t size, uint8_t* data)
			: Command(code), mSize(size)
		{
			mData = new uint8_t[size];
			memcpy(mData, data, size);
		}
		
		virtual ~AnyCommand()
		{
			delete[] mData;
		}
		
		virtual uint16_t  Size() { return mSize; }
		virtual uint8_t*  Data() { return mData; }

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

	private:
		uint16_t  mSize;
		uint8_t*  mData;
};/*}}}*/

/*
 * Wrap AnyCommand
 */
class AggregatedCommand : public Command/*{{{*/
{
	public:
		AggregatedCommand(AnyCommand* command)
			: Command(command->Code()), mCommand(command)
		{ }

		virtual ~AggregatedCommand()
		{
			delete mCommand;
		}

		virtual uint16_t  Size() { return mCommand->Size(); };
		virtual uint8_t*  Data() { return mCommand->Data(); };

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

	private:
		AnyCommand* mCommand;

};/*}}}*/

class Command_65 : public Command/*{{{*/
{
	public:
		Command_65(uint32_t type, uint32_t oid1, uint32_t oid2)
			: Command(0x65)
		{
			mData.type = type;
			mData.oid1 = oid1;
			mData.oid2 = oid2;
			mData.unknown = 2;
		}
			
		virtual uint16_t  Size() { return sizeof(mData); };
		virtual uint8_t*  Data() { return reinterpret_cast<uint8_t*>(&mData); };

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

	private:
		struct Packet
		{
			uint32_t type;
			uint32_t oid1;
			uint32_t oid2;
			uint32_t unknown;
		};

		Packet mData;
};/*}}}*/

class Command_67 : public Command/*{{{*/
{
	public:
		Command_67(uint32_t type, uint32_t oid)
			: Command(0x67)
		{
			mData.empty = 0;
			mData.type = type;
			mData.unknown = 1; /* probably id count */
			mData.oid = oid;
		}
			
		virtual uint16_t  Size() { return sizeof(mData); };
		virtual uint8_t*  Data() { return reinterpret_cast<uint8_t*>(&mData); };

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

	private:
		struct Packet
		{
			uint32_t empty;
			uint32_t type;
			uint32_t unknown;
			uint32_t oid;
		};

		Packet mData;
};/*}}}*/

class Command_6f : public Command/*{{{*/
{
	public:
		Command_6f(uint32_t subcommand)
			: Command(0x6f), mSubcommand(subcommand)
		{
		}
		
		virtual uint16_t  Size() { return sizeof(mSubcommand); };
		virtual uint8_t*  Data() { return reinterpret_cast<uint8_t*>(&mSubcommand); };

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

	private:
		uint32_t mSubcommand;
};/*}}}*/

class Command_70_2 : public Command/*{{{*/
{
	public:
		Command_70_2(uint32_t subsubcommand)
			: Command(0x70)
		{
			mData.size2       = sizeof(mData) - sizeof(mData.size2);
			mData.unknown1    = 0xf0000001;
			mData.subcommand  = 2;
			memset(mData.empty1, 0, sizeof(mData.empty1));
			mData.subsubcommand = subsubcommand;

			if (1 == subsubcommand)
				mData.unknown2 = 0x80000003;
			else
				mData.unknown2 = 0;
			
			memset(mData.empty2, 0, sizeof(mData.empty2));
		}
			
		virtual uint16_t  Size() { return sizeof(mData); };
		virtual uint8_t*  Data() { return reinterpret_cast<uint8_t*>(&mData); };

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

	private:
		struct Packet
		{
			uint32_t size2;
			uint32_t unknown1;
			uint32_t subcommand;
			uint8_t empty1[0xc8];
			uint32_t subsubcommand;
			uint32_t unknown2;
			uint8_t empty2[0x18];
		};

		Packet mData;
};/*}}}*/

class Command_70_3 : public Command/*{{{*/
{
	public:
		Command_70_3(uint32_t count, uint32_t* types)
			: Command(0x70), mCount(count)
		{
			mData = new uint32_t[8 + count];

			mData[0] = (7 + count) * sizeof(uint32_t);   // remaining size
			mData[1] = 0xf0000001;                       // unknown
			mData[2] = 3;                                // subcommand
			mData[3] = 2;                                // unknown...
			mData[4] = 0;
			mData[5] = 0;
			mData[6] = 0;
			mData[7] = count;
			
			memcpy(mData + 8, types, count * sizeof(uint32_t));
		}

		virtual ~Command_70_3()
		{
			delete[] mData;
		}

		virtual uint16_t  Size() { return mData[0] + sizeof(mData[0]); };
		virtual uint8_t*  Data() { return reinterpret_cast<uint8_t*>(mData); };

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

	private:
		uint32_t mCount;
		uint32_t* mData;
};/*}}}*/

class ObjectIdList : public Command/*{{{*/
{
	public:
		/*
		 * Create command from buffer
		 */
		ObjectIdList(AnyCommand* command)
			: Command(command->Code()), 
				mModifiedCount(0), mDeletedCount(0),
				mModified(NULL), mDeleted(NULL)
		{
			if (command->Code() != 0x0069)
			{
				synce_error("Invalid command code");
				return;
			}
			
			if (command->Size() < 0x10)
			{
				synce_error("Too little data");
				return;
			}

			uint32_t* values = (uint32_t*)command->Data();

			if ((values[3] + 0x10) != command->Size())
			{
				synce_error("Invalid data");
				abort();
			}
			
			mSubcommand     = letoh32(values[0]);
			mType           = letoh32(values[1]);
			mModifiedCount  = letoh32(values[2]);
			mModified  = new uint32_t[mModifiedCount];

			unsigned i = 4;

			for (unsigned j = 0; j < mModifiedCount; j++, i++)
				mModified[j] = letoh32(values[i]);

			mDeletedCount   = letoh32(values[3]) / sizeof(uint32_t) - mModifiedCount;
			mDeleted   = new uint32_t[mDeletedCount];

			for (unsigned j = 0; j < mDeletedCount; j++, i++)
				mDeleted[j] = letoh32(values[i]);
		}

		/*
		 * Create list with maximum one modified element and one deleted
		 */
		ObjectIdList(
				uint32_t subcommand, 
				uint32_t type, 
				uint32_t modified, 
				uint32_t deleted)
			: Command(0x0069), 
				mModifiedCount(0), mDeletedCount(0),
				mModified(NULL), mDeleted(NULL)
		{
			mSubcommand     = subcommand;
			mType           = type;
			mModifiedCount  = modified ? 1 : 0;
			mDeletedCount   = deleted ? 1 : 0;

			if (mModifiedCount)
			{
				mModified = new uint32_t[mModifiedCount];
				mModified[0] = modified;
			}
			
			if (mDeletedCount)
			{
				mDeleted = new uint32_t[mDeletedCount];
				mDeleted[0] = deleted;
			}
		}

		virtual ~ObjectIdList()
		{
			delete mModified;
			delete mDeleted;
			delete mData;
		}

		virtual uint16_t  Size() 
		{ 
			return (4 + mModifiedCount + mDeletedCount) * sizeof(uint32_t); 
		};

		virtual uint8_t*  Data() 
		{
			if (mData)
				delete mData;
		
			mData = new uint8_t[Size()];
			uint32_t* values = (uint32_t*)mData;
			
			values[0] = htole32(mSubcommand);
			values[1] = htole32(mType);
			values[2] = htole32(mModifiedCount);
			values[3] = htole32((mModifiedCount + mDeletedCount) * sizeof(uint32_t));

			unsigned i = 4;

			for (unsigned j = 0; j < mModifiedCount; j++, i++)
				values[i] = htole32(mModified[j]);

			for (unsigned j = 0; j < mDeletedCount; j++, i++)
				values[i] = htole32(mDeleted[j]);
			
			return mData;
		}

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

		uint32_t Subcommand()     { return mSubcommand; }
		uint32_t Type()           { return mType; }
		
		uint32_t ModifiedCount()  { return mModifiedCount; } 
		uint32_t DeletedCount()   { return mDeletedCount; }

		uint32_t Modified(unsigned i)      { return mModified[i]; }
		uint32_t Deleted(unsigned i)       { return mDeleted[i]; }

	public:
		uint32_t mSubcommand;
		uint32_t mType;
		
		uint32_t mModifiedCount;
		uint32_t mDeletedCount;

		uint32_t* mModified;
		uint32_t* mDeleted;

		uint8_t* mData;

};/*}}}*/

class Reply : public AggregatedCommand/*{{{*/
{
	public:
		Reply(AnyCommand* command)
			: AggregatedCommand(command)
		{
			if (Code() != 0x6c)
			{
				synce_error("Incompatible packet");
				throw Error();
			}
		}

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

		uint32_t InReplyTo() 
		{
			return letoh32(*(uint32_t*)(Data())); 
		}

};/*}}}*/

class ObjectTypeList : public Reply /*{{{*/
{
	public:
		ObjectTypeList(AnyCommand* command)
			: Reply(command)
		{
			if (InReplyTo() != 0x6f)
			{
				synce_error("Incompatible packet");
				throw Error();
			}

			// TODO: verify more of packet
		}

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}

		uint32_t Length() 
		{ 
			return letoh32(*(uint32_t*)(Data() + 0x20)); 
		}

		ObjectTypeRecord* Item(unsigned index)
		{
			return ((ObjectTypeRecord*)(Data() + 0x24)) + index;
		}
};/*}}}*/

class ReplyToCommand_70 : public Reply /*{{{*/
{
	public:
		ReplyToCommand_70(AnyCommand* command)
			: Reply(command)
		{
			if (InReplyTo() != 0x70)
			{
				synce_error("Incompatible packet");
				throw Error();
			}

			if (Size() != 0x10)
			{
				synce_warning("Packet size not 0x10 but 0x%08x", Size());
			}
		}

		virtual void Dispatch(ReplicationAgent& ra)
		{
			ra.OnCommand(*this);
		}
};/*}}}*/

void ReplicationAgent::Start()/*{{{*/
{
	struct sockaddr_in address;

	mServer.Listen(NULL, RRAC_PORT);
	CeStartReplication();

	mpCmdChannel   = mServer.Accept(address);
	mpDataChannel  = mServer.Accept(address);

	// Ask for object type list
	{
		Command_6f command(0xc1);
		WriteCommand(command);
	}

	// Receive object type list
	ObjectTypeList* object_type_list = new ObjectTypeList(ReadCommand());

	synce_trace("Got object type list with %i items", object_type_list->Length());

	// Ignore everything but contacts
	uint32_t* type_ids = new uint32_t[object_type_list->Length()];

	unsigned j = 0;
	for (unsigned i = 0; i < object_type_list->Length(); i++)
	{
		uint32_t type_id = object_type_list->Item(i)->type_id;

		if (type_id != 0x2712)
		{
			type_ids[j++] = type_id;
		}
	}

	{
		Command_70_3 command(j, type_ids);
		WriteCommand(command);
	}

	delete[] type_ids;
	
	// Ignore reply
	delete new ReplyToCommand_70(ReadCommand());
	
	DispatchCommand(ReadCommand());
	DispatchCommand(ReadCommand());
	DispatchCommand(ReadCommand());
	
#if 1
	{
#if 0
		{
			Command_70_2 command(1);
			WriteCommand(command);
		}

		// Ignore reply
		delete new ReplyToCommand_70(ReadCommand());
#endif
		uint8_t buffer[] = 
		{
			0xff, 0xff, 0xff, 0xff, 0x12, 0x27, 0x00, 0x00, // type
			0x02, 0x00, 0x00, 0x00, 0xff, 0xff, 0xa0, 0xff, // size and unknown
			0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // field count,
			0x1f, 0x00, 0x06, 0x3a, 'X',    0,  'a',    0,  // propid
			 'v',    0,  'i',    0, 'd',    0,    0,    0,
			0x1f, 0x00, 0x11, 0x3a, // propid
       'X',    0,  'a',    0, 'v',    0,  'i',    0,
			 'd',    0,    0,    0,
			0x1f, 0x00, 0x13, 0x40, // propid
       'X',    0,  'a',    0, 'v',    0,  'i',    0,
			 'd',    0,    0,    0,
#if 0
			 0x1f, 0x04, 0x45, 0x40, 0x1f, 0x04, 0x46, 0x40,
			 0x1f, 0x04, 0x47, 0x40, 0x1f, 0x04, 0x48, 0x40,
			 0x1f, 0x04, 0x49, 0x40, 0x1f, 0x04, 0x83, 0x40,
			 0x1f, 0x04, 0x93, 0x40, 0x1f, 0x04, 0xa3, 0x40,
			 0x1f, 0x04, 0x17, 0x43, 0x1f, 0x04, 0x08, 0x40,
			 0x1f, 0x04, 0x40, 0x40, 0x1f, 0x04, 0x41, 0x40,
			 0x1f, 0x04, 0x42, 0x40, 0x1f, 0x04, 0x43, 0x40,
			 0x1f, 0x04, 0x44, 0x40, 0x1f, 0x04, 0x4a, 0x40,
			 0x1f, 0x04, 0x4b, 0x40, 0x1f, 0x04, 0x4c, 0x40,
			 0x1f, 0x04, 0x4d, 0x40, 0x1f, 0x04, 0x4e, 0x40,
			 0x1f, 0x04, 0x23, 0x40, 0x1f, 0x04, 0x05, 0x3a,
			 0x1f, 0x04, 0x24, 0x40, 0x1f, 0x04, 0x02, 0x40,
			 0x1f, 0x04, 0x0a, 0x40, 0x1f, 0x04, 0x06, 0x40,
			 0x1f, 0x04, 0x17, 0x3a, 0x1f, 0x04, 0x16, 0x3a,
			 0x1f, 0x04, 0x18, 0x3a, 0x1f, 0x04, 0x19, 0x3a,
			 0x40, 0x04, 0x03, 0x40, 0x40, 0x04, 0x01, 0x40,
			 0x41, 0x04, 0x17, 0x00, 0x1f, 0x04, 0x05, 0x40,
			 0x1f, 0x04, 0x08, 0x3a, 0x1f, 0x04, 0x07, 0x40,
			 0x1f, 0x04, 0x24, 0x3a, 0x1f, 0x04, 0x09, 0x3a,
			 0x1f, 0x04, 0x2f, 0x3a, 0x1f, 0x04, 0x1c, 0x3a,
			 0x1f, 0x04, 0x09, 0x40, 0x1f, 0x04, 0x1e, 0x3a,
			 0x1f, 0x04, 0x25, 0x3a, 0x1f, 0x04, 0x04, 0x40,
			 0x1f, 0x04, 0x1d, 0x3a
#endif
#if 0
			0xff, 0xff, 0xff, 0xff,
			0x12, 0x27, 0x00, 0x00, // type
			0x00, 0x00, 0x00, 0x00
#endif
		};
		
#if 0
		ObjectIdList command(0, 0x2712, 222, 0);
		WriteCommand(command);
#endif
		*(uint32_t*)(buffer + 0) = (uint32_t)time(NULL);
		*(uint16_t*)(buffer + 12) = sizeof(buffer) - 0x12;

		dump("Data", buffer, sizeof(buffer));
		mpDataChannel->Write(buffer, sizeof(buffer));
	}
#endif
		
}/*}}}*/

void ReplicationAgent::EventLoop()/*{{{*/
{
	synce_trace("Entering event loop");

	for (;;)
	{
		SocketEvents events = EVENT_READ;
		mpCmdChannel->Wait(-1, events);

		if (events & EVENT_READ)
		{
			DispatchCommand( ReadCommand() );
		}
	}
}/*}}}*/
		
AnyCommand* ReplicationAgent::ReadCommand()/*{{{*/
{
	uint16_t code = mpCmdChannel->Read16();
	uint16_t size = mpCmdChannel->Read16();
	uint8_t* data = mpCmdChannel->Read(size);
	synce_trace("Read command %04x with data size %08x", code, size);
	dump("Command data", data, size);
	return new AnyCommand(code, size, data);
}/*}}}*/

void ReplicationAgent::DispatchCommand(AnyCommand* anyCommand)/*{{{*/
{
	Command* command = anyCommand;
	
	switch (anyCommand->Code())
	{
		case 0x0069:
			{
				if (anyCommand->Size() < 0x10)
				{
					synce_warning("Suspicius data size for command %04x: %08x", 
							anyCommand->Code(), anyCommand->Size());
				}
				
				uint32_t subcommand = letoh32(*(uint32_t*)anyCommand->Data());

				switch (subcommand)
				{
					case 0x00000000:
					case 0x04000000:
					case 0x06000000:
						command = new ObjectIdList(anyCommand);
						break;
						
					case 0x02000000:
						break;
				}
				
			}
			break;

		case 0x0065:
			{
				Command_65 command(
						letoh32(*(uint32_t*)(anyCommand->Data() + 0x00)),
						letoh32(*(uint32_t*)(anyCommand->Data() + 0x08)),
						letoh32(*(uint32_t*)(anyCommand->Data() + 0x08)));
				WriteCommand(command);
			}
			break;

		default:
			break;
	}

	command->Dispatch(*this);

	if (command != anyCommand)
		delete command;
	delete anyCommand;
}/*}}}*/
		
void ReplicationAgent::WriteCommand(Command& command)/*{{{*/
{
	size_t buffer_size = command.Size() + 4;
	uint8_t* buffer = new uint8_t[buffer_size];

	*(uint16_t*)(buffer + 0) = htole16(command.Code());
	*(uint16_t*)(buffer + 2) = htole16(command.Size());
	memcpy(buffer + 4, command.Data(), command.Size());

	synce_trace("Writing command %04x with data size %08x", 
			command.Code(), command.Size());
	dump("Command data", buffer + 4, command.Size());

	mpCmdChannel->Write(buffer, buffer_size);

	delete[] buffer;
}/*}}}*/
		
void ReplicationAgent::OnCommand (ObjectIdList& objectIdList)
{
	if (objectIdList.Subcommand() == 0)
	{
#if 0
		{
			Command_70_2 command(1);
			WriteCommand(command);
		}

		// Ignore reply
		delete new ReplyToCommand_70(ReadCommand());
#endif
		
		if (objectIdList.ModifiedCount())
		{
			Command_67 command(objectIdList.Type(), objectIdList.Modified(0));
			//Command_67 command(objectIdList.Type(), 0x8000e07);
			WriteCommand(command);

			SocketEvents events = EVENT_READ;
			mpDataChannel->Wait(-1, events);

			if (!(events & EVENT_READ))
			{
				synce_error("Unexpected events: %04x", events);
			}
			
			uint32_t available = mpDataChannel->Available();
			uint8_t* data = mpDataChannel->Read(available);

			dump("Data", data, available);

			uint32_t oid   = letoh32(*(uint32_t*)(data + 0x00));
			uint32_t type  = letoh32(*(uint32_t*)(data + 0x04));
			uint16_t size  = letoh16(*(uint16_t*)(data + 0xc));

			if (0x2712 == type)
			{
				/* contacts */
				uint32_t field_count = letoh32(*(uint32_t*)(data + 0x10));
				CEPROPVAL* propvals = new CEPROPVAL[field_count];

				if (dbstream_to_propvals(data + 0x18, field_count, propvals))
				{
					char* vcard = NULL;
					if (contact_to_vcard(oid, propvals, field_count, &vcard))
					{
						synce_trace("vCard: '%s'", vcard);
						free(vcard);
					}
				}

				delete[] propvals;
			}

			delete[] data;
			synce_trace("oid=%08x, type=%08x", oid, type);

			{
				Command_65 command(type, oid, oid);
				WriteCommand(command);
			}
		}
		
#if 0
		{
			Command_70_2 command(2);
			WriteCommand(command);
		}
#endif

		{
			Command_6f command(6);
			WriteCommand(command);
		}

#if 0
		// Ignore reply
		delete new ReplyToCommand_70(ReadCommand());
#endif
	
		// Ignore reply
		delete ReadCommand();
	}
}

int main(int argc, char** argv)
{
	int result = 1;
	ReplicationAgent ra;

	if ( FAILED(CeRapiInit()) )
		return 1;

	try
	{	
		ra.Start();
		ra.EventLoop();

		result = 0;
	}
	catch (ListenFailed& e)
	{	
		synce_error("Failed to start socket server");
	}
	catch (...)
	{
		synce_error("Unknown error");
	}

	CeRapiUninit();
	
	return result;
}
