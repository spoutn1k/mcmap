#include <cstdio>
#include <iostream>
#include <cstdlib>

// For MSVC++, get "zlib compiled DLL" from http://www.zlib.net/ and read USAGE.txt
// gcc from MinGW works with the static library of zlib; however, trying
// static linking in MSVC++ gave me weird results and crashes (v2008)
// on linux, static linking works too, of course, but shouldn't be needed

#include <zlib.h>
#include "nbt.h"

#define ID_SIZE 1
#define NAME_LENGTH_SIZE 2

NBT::NBT(const char *file, bool &success) {
	int ret;

	_blob = NULL;
	_filename = NULL;
	gzFile fh = 0;
	success = true;

	if (file == NULL || *file == '\0' || !fileExists(file)) {
		success = false;
		return;
	}
	_filename = strdup(file);

	if ((fh = gzopen(file, "rb")) == 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		success = false;
		return;
	}

	// File is open, read data
	// I checked a few chunk files in their decompressed form,
	// I think they can't really get any bigger than 80~90KiB, but just to be sure...
	int length = 150000 * (CHUNKSIZE_Y / 128);
	_blob = new uint8_t[length];

	if ((ret = gzread(fh, _blob, length)) == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
		success = false;
		return;
	}

	_bloblen = ret;
	gzclose(fh);

	if (_blob[0] != T_COMPOUND) { // Has to start with TAG_Compound
		fprintf(stderr, "Malformed NTB file: begins by type %d\n", _type);
		success = false;
		return;
	}

	// Skip type and name of the root tag
	uint8_t *position = _blob + ID_SIZE + NAME_LENGTH_SIZE, *end = _blob + ret;
	_type = T_COMPOUND;

	if (!NBT_Tag::parseData(position, end)) {
		fprintf(stderr, "Error parsing the NBT data\n");
		success = false;
		return;
	}

	if (position == NULL) {
		fprintf(stderr, "Error reading the file\n");
		success = false;
		return;
	}
}

NBT::NBT(uint8_t * const file, const size_t len, const bool shared, bool &success) {
	_filename = NULL;
	if (shared) {
		_blob = NULL;
	} else {
		_blob = new uint8_t[len];
		memcpy(_blob, file, len);
	}
	_bloblen = len;
	uint8_t *position = file + ID_SIZE + NAME_LENGTH_SIZE, *end = file + len;
	_type = T_COMPOUND;

	if (!NBT_Tag::parseData(position, end)) {
		fprintf(stderr, "Error parsing the NBT data\n");
		success = false;
		return;
	}

	if (position == NULL) {
		fprintf(stderr, "Error reading the file\n");
		success = false;
		return;
	}
	success = (position != NULL);
}

NBT::~NBT() {
	if (_blob != NULL)
		delete[] _blob;

	if (_filename != NULL)
		free(_filename);
}

/*
   bool NBT::save()
   {	// While loading nbt files while the server is running works in most
// cases, it is strongly advised that you only save map chunks while
// the game/server is not running.
if (_filename == NULL) return false;
char *tmpfile = new char[strlen(_filename) + 5];
strcpy(tmpfile, _filename);
strcat(tmpfile, ".bak");
int ret = rename(_filename, tmpfile);
if (ret != 0) {
delete[] tmpfile;
return false;
}
gzFile fh = gzopen(_filename, "w6b");
if (fh == 0) {
delete[] tmpfile;
return false;
}
ret = gzwrite(fh, _blob, (unsigned int)_bloblen);
gzclose(fh);
if (ret != _bloblen) {
rename(tmpfile, _filename);
delete[] tmpfile;
return false;
}
//remove(tmpfile);
delete[] tmpfile;
//fprintf(stderr, "Saved %s\n", _filename);
return true;
}
*/

//  _   _ ____ _____ _____           
// | \ | | __ )_   _|_   _|_ _  __ _ 
// |  \| |  _ \ | |   | |/ _` |/ _` |
// | |\  | |_) || |   | | (_| | (_| |
// |_| \_|____/ |_|___|_|\__,_|\__, |
//				 |_____|       |___/ 

NBT_Tag::NBT_Tag() : _compound_content{}, _list_content{}, _name("") {
	_data = NULL;
	_type = T_UNKNOWN;
	_name_length = 0;
	_len = 0;
}

NBT_Tag::NBT_Tag(uint8_t* &position, const uint8_t *end) : NBT_Tag() {
	// Why was that arbitrary 11 here ?!
	//if (*position < 1 || *position > 11 || end - position < 3) {
	if (*position < 1 || end - position < ID_SIZE + NAME_LENGTH_SIZE) {
		position = NULL;
		return;
	}

	_type = (TagType)*position;
	_name_length = _ntohs(position+1);

	if (end - position < _name_length) {
		position = NULL;
		return;
	}

	/*_name = (char*) malloc(_name_length*sizeof(char)+1);
	memcpy(_name, (char *)(position + ID_SIZE + NAME_LENGTH_SIZE), _name_length);
	_name[_name_length] = '\0';*/
	_name = string((char *)(position + ID_SIZE + NAME_LENGTH_SIZE), _name_length);

	position += 3 + _name_length;

	if (!parseData(position, end)) {
		fprintf(stderr, "Error parsing the NBT data\n");
		return;
	}
}

NBT_Tag::NBT_Tag(uint8_t* &position, const uint8_t *end, TagType type) : NBT_Tag() {
	// this contructor is used for TAG_List entries
	_type = type;

	if (!parseData(position, end)) {
		fprintf(stderr, "Error parsing the NBT data\n");
		return;
	}
}

void import(uint8_t** buffer, uint8_t* data, size_t length) {
	*buffer = (uint8_t*) malloc(length*sizeof(uint8_t));
	memcpy(*buffer, data, length);
}

bool NBT_Tag::parseData(uint8_t* &position, const uint8_t *end) {
	// position should now point to start of data (for named tags, right after the name)
	uint32_t count;
	switch (_type) {
		case T_COMPOUND:
			while (*position != 0 && position < end) { // No end tag, go on...
				NBT_Tag tmp = NBT_Tag(position, end);
				if (position == NULL) {
					return false;
				}
				_compound_content[tmp._name] = tmp;
			}
			++position;
			break;

		case T_LIST:
			if (*position < 0 || *position > 11) {
				position = NULL;
				return false;
			}
			TagType type;
			if (*position == 0)
				type = (TagType)1; // ((*position)+1);
			else
				type = (TagType)*position;
			count = _ntohl(position+1);
			position += 5;
			while (count-- && position < end) { // No end tag, go on...
				NBT_Tag tmp = NBT_Tag(position, end, type);
				if (position == NULL) {
					return false;
				}
				_list_content.push_back(tmp);
			}
			break;

		case T_BYTE:
			import(&_data, position, 1);
			_len = 1;
			position += 1;
			break;

		case T_SHORT:
			import(&_data, position, 2);
			_len = 2;
			position += 2;
			break;

		case T_INT:
			import(&_data, position, 4);
			_len = 4;
			position += 4;
			break;

		case T_LONG:
			import(&_data, position, 8);
			_len = 8;
			position += 8;
			break;

		case T_FLOAT:
			import(&_data, position, 4);
			_len = 4;
			position += 4;
			break;

		case T_DOUBLE:
			import(&_data, position, 8);
			_len = 8;
			position += 8;
			break;

		case T_BYTEARRAY:
			_len = _ntohl(position);
			if (position + _len + 4 >= end) {
				fprintf(stderr, "ByteArray too long by %d bytes!\n", int((position + _len + 4) - end));
				position = NULL;
				return false;
			}
			_data = position + 4;
			position += 4 + _len;
			break;

		case T_STRING:
			_len = _ntohs(position);
			if (position + _len + 2 >= end) {
				fprintf(stderr, "Malformed NBT Tag: Too long string !\n");
				position = NULL;
				return false;
			}

			// TODO Find a better fix to insert the '\0' at the end of the string
			import(&_data, position+2, _len+1);

			_data[_len] = '\0';
			position += 2 + _len;
			break;

		case T_INTARRAY:
			{
				// A long array is an int followed by k ints
				uint8_t header_size = 4, element_size = 4;
				_len = _ntohl(position) * element_size;
				if (position + _len + header_size >= end) {
					fprintf(stderr, "IntArray too long by %d bytes!\n",
							int((position + _len + header_size) - end));
					position = NULL;
					return false;
				}
				_data = position + header_size;
				position += header_size + _len;
				break;
			}

		case T_LONGARRAY:
			{
				// A long array is an int followed by k longs
				uint8_t header_size = 4, element_size = 8;
				_len = _ntohl(position) * element_size;
				if (position + _len + header_size >= end) {
					fprintf(stderr, "LongArray too long by %d bytes!\n",
							int((position + _len + header_size) - end));
					position = NULL;
					return false;
				}
				_data = position + header_size;
				position += header_size + _len;
				break;
			}

		case T_UNKNOWN:
		default:
			fprintf(stderr, "Unknown tag: %d!\n", (int)_type);
			position = NULL;
			break;
	}

	return true;
}

NBT_Tag::~NBT_Tag() {
	/*if (_compound_content) {
		for (nbt_map::iterator it = _compound_content->begin(); it != _compound_content->end(); it++) {
			delete (it->second);
		}
		//delete _compound_content;
	}

	if (_list_content) {
		for (nbt_listend(); it++) {
			delete *it;
		}
		//delete _list_content;
	}

	if (_name_length) {
		//free(_name);
	}

	// Freeing _data is tricky as some tags use mallocs and some tags 
	// have links to the original blob. This makes sure all free's are
	// done on valid, malloc'd addresses.
	if (_len && (_type != T_COMPOUND 
				&& _type != T_LIST 
				&& _type != T_BYTEARRAY 
				&& _type != T_INTARRAY 
				&& _type != T_LONGARRAY)) {
		//free(_data);
	}*/
}

bool NBT_Tag::getByte(int8_t &value) {
	if (_type != T_BYTE) {
		fprintf(stderr, "Accessing byte of incompatible NBT_Tag\n");
		return 0;
	}
	value = *_data;
	return true;
}

bool NBT_Tag::getShort(int16_t &value) {
	if (_type != T_SHORT) {
		fprintf(stderr, "Accessing short of incompatible NBT_Tag\n");
		return false;
	}

	value = _ntohs(_data);
	return true;
}

bool NBT_Tag::getInt(int32_t &value) {
	if (_type != T_INT) {
		fprintf(stderr, "Accessing int of incompatible NBT_Tag\n");
		return false;
	}

	value = _ntohl(_data);
	return true;
}

bool NBT_Tag::getLong(int64_t &value) {
	if (_type != T_LONG) {
		fprintf(stderr, "Accessing long of incompatible NBT_Tag\n");
		return false;
	}

	value = _ntohll(_data);
	return true;
}

bool NBT_Tag::getFloat(float &value) {
	if (_type != T_FLOAT) {
		fprintf(stderr, "Accessing float of incompatible NBT_Tag\n");
		return false;
	}

	int32_t tmp = _ntohl(_data);
	value = *((float*) &tmp);
	return true;
}

bool NBT_Tag::getDouble(double &value) {
	if (_type != T_DOUBLE) {
		fprintf(stderr, "Accessing double of incompatible NBT_Tag\n");
		return false;
	}

	int64_t tmp = _ntohll(_data);
	value = *((double*) &tmp);
	return true;
}

bool NBT_Tag::getByteArray(uint8_t* &data, uint32_t &len) {
	if (_type != T_BYTEARRAY) {
		fprintf(stderr, "Accessing byte array of incompatible NBT_Tag\n");
		return false;
	}

	data = _data;
	len = _len;
	return true;
}

bool NBT_Tag::getString(string &value) {
	if (_type != T_STRING) {
		fprintf(stderr, "Accessing string of incompatible NBT_Tag: %s\n", getName().c_str());
		return false;
	}

	value = string((char*) _data);
	return true;
}

bool NBT_Tag::getList(nbt_list &value) {
	if (_type != T_LIST) {
		fprintf(stderr, "Accessing list of incompatible NBT_Tag\n");
		return false;
	}

	value = _list_content;
	return true;
}

bool NBT_Tag::getCompound(nbt_map &compound) {
	if (_type != T_COMPOUND) {
		fprintf(stderr, "Accessing compound map of incompatible NBT_Tag\n");
		return false;
	}

	compound = _compound_content;
	return true;
}

bool NBT_Tag::getIntArray(uint8_t* &data, uint32_t &len) {
	if (_type != T_INTARRAY) {
		fprintf(stderr, "Accessing int array of incompatible NBT_Tag\n");
		return false;
	}

	data = _data;
	len = _len;
	return true;
}

bool NBT_Tag::getLongArray(uint8_t* &data, uint32_t &len) {
	if (_type != T_LONGARRAY) {
		fprintf(stderr, "Accessing long array of incompatible NBT_Tag\n");
		return false;
	}

	data = _data;
	len = _len;
	return true;
}

string getBlockId(uint64_t position, NBT_Tag section) {
	uint64_t size = (uint64_t) ceil(log2(section["Palette"].getList().size()));
	size = (4 > size ? 4 : size);
	uint8_t* raw_data = section["BlockStates"]._data;
	uint64_t skip = position*size/64; // The number of longs to skip
	int64_t remain = position*size - 64*skip; // The bits to skip in the first non-skipped long
	int64_t overflow = remain + size - 64; // The bits in the first non-skipped long

	uint64_t mask = ((1l << size) - 1) << remain;
	uint64_t lower_data = (_ntohll(raw_data + skip*8) & mask) >> remain;

	if (overflow > 0) {
		uint64_t upper_data = _ntohll(raw_data + (skip+1)*8) & ((1l << overflow) -1);
		lower_data = lower_data | upper_data << (size - overflow);
	}

	return section["Palette"][lower_data]["Name"].getString();
}
