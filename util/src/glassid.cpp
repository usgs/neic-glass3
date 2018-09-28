#include <glassid.h>

#ifdef _WIN32
#include <windows.h>
#pragma warning(disable : 4996)
#else
#include <uuid/uuid.h>
typedef unsigned char byte;
#endif

#include <memory.h>
#include <string>

namespace glass3 {
namespace util {

// ---------------------------------------------------------GlassID
GlassID::GlassID() {
}

// ---------------------------------------------------------~GlassID
GlassID::~GlassID() {
}

// ---------------------------------------------------------getID
std::string GlassID::getID() {
	// get guid (also known as uuid in linux)
#ifdef _WIN32
	GUID guid;
	CoCreateGuid(&guid);
#else
	uuid_t guid;
	uuid_generate_random(guid);
#endif

	// get byte representation of guid
	byte * c = reinterpret_cast<byte *>(&guid);

	// create id string using byte representation
	char pid[40];
	snprintf(pid, sizeof(pid),
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			c[3], c[2], c[1], c[0], c[5], c[4], c[7], c[6], c[8], c[9], c[10],
			c[11], c[12], c[13], c[14], c[15]);

	// make sure the whole id string is upper case
	for (int j = 0; j < 32; j++) {
		pid[j] = toupper(pid[j]);
	}

	// null terminate the id string
	pid[32] = 0;

	// return id string
	return (pid);
}

// ---------------------------------------------------------random
unsigned int GlassID::random() {
	// get guid (also known as uuid in linux)
#ifdef _WIN32
	GUID guid;
	CoCreateGuid(&guid);
#else
	uuid_t guid;
	uuid_generate_random(guid);
#endif

	// get byte representation of guid
	byte * c = reinterpret_cast<byte *>(&guid);

	// create integer id using byte representation
	unsigned int random;
	memcpy(&random, &c[6], sizeof(unsigned int));

	// return integer id
	return (random);
}
}  // namespace util
}  // namespace glass3
