/***
 * Tiny OO NBT parser for C++
 * v1.0, 09-2010 by Zahl
 */
#include <cstdio>
#include <cstdlib>


// For MSVC++, get "zlib compiled DLL" from http://www.zlib.net/ and read USAGE.txt
// gcc from MinGW works with the static library of zlib; however, trying
// static linking in MSVC++ gave me weird results and crashes (v2008)
// on linux, static linking works too, of course, but shouldn't be needed

#include <zlib.h>

#include "nbt.h"

/* A few words on this class:
 * It is written in a half-way OO manner. That is, it breaks
 * some best practice rules. For example, for speed and efficiency
 * reasons, it shares memory across classes and even returns pointers
 * to memory locations inside the memory block owned by the NBT class.
 * That's why you should keep two things in mind:
 * 1) The pointer returned by getByteArray() lies inside a memory
 * block that belongs to the NBT class. NEVER try to delete it, and
 * do NOT use it anymore after you deleted the instance of NBT (or
 * after you left the scope of a local instance)
 * 2) Pretty much the same thing goes for getCompound(). The pointer
 * to the NBT_Tag instance returned by it is owned by the root NBT
 * object. NEVER delete it on your own, and do NOT use it anymore
 * after you deleted the root NBT instance.
 *
 * Rule of thumb: Only use the pointer returned by getByteArray
 * or getCompound in a temporary context, never store it unless
 * you know what you're doing.
 * All the other get-methods return a copy of the requested data
 * (if found), so you're safe here...
 *
 * There is no way (yet) to list all available tags, simply because
 * it isn't needed here. It shouldn't be too much work to add that
 * if you need it.
 *
 * --
 * You may use and modify this class in you own projects, just
 * keep a note in your source code that you used/modified my class.
 * And make your tool open source of course.
 * Thanks :-)
 */

// I feel like including these just for ntohs/ntohl is unnecessary,
// especially as you need two targets in the makefile then (unix and windows)
/*
#if defined(linux) || defined(unix)
#include <arpa/inet.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif
*/
// Using these two functions instead
static inline uint16_t _ntohs(uint8_t *val)
{
	return (uint16_t(val[0]) << 8)
	       + (uint16_t(val[1]));
}

static inline uint32_t _ntohl(uint8_t *val)
{
	return (uint32_t(val[0]) << 24)
	       + (uint32_t(val[1]) << 16)
	       + (uint32_t(val[2]) << 8)
	       + (uint32_t(val[3]));
}

NBT::NBT()
{
	_blob = NULL;
	_filename = NULL;
}

///-------------------------------------------

NBT::NBT(const char *file, bool &success)
{
	_blob = NULL;
	_filename = NULL;
	if (file == NULL || *file == '\0' || !fileExists(file)) {
		//fprintf(stderr, "File not found: %s\n", file);
		success = false;
		return;
	}
	_filename = strdup(file);
	gzFile fh = 0;
	for (int i = 0; i < 20; ++i) { // Give up eventually if you can't open the file....
		if ((fh = gzopen(file, "rb")) != 0) {
			break;
		}
		usleep(5000); // sleep 5ms
	}
	success = (fh != 0);
	if (!success) {
		//fprintf(stderr, "Error opening file %s\n", file);
		return;
	}
	// File is open, read data
	int length = 150000 * (CHUNKSIZE_Y / 128); // I checked a few chunk files in their decompressed form, I think they can't really get any bigger than 80~90KiB, but just to be sure...
	_blob = new uint8_t[length];
	int ret = gzread(fh, _blob, length);
	gzclose(fh);
	if (ret == -1 || _blob[0] != 10) { // Has to start with TAG_Compound
		//fprintf(stderr, "Ret: %d/%d - start: %d\n", ret, length, (int)_blob[0]);
		success = false;
		return;
	}
	_bloblen = ret;
	uint8_t *position = _blob + 3, *end = _blob + ret;
	_type = tagCompound;
	NBT_Tag::parseData(position, end);
	if (position == NULL) {
		//fprintf(stderr, "Error parsing NBT from file\n");
	}
	success = (position != NULL);
}

NBT::NBT(uint8_t * const file, const size_t len, const bool shared, bool &success)
{
	_filename = NULL;
	if (shared) {
		_blob = NULL;
	} else {
		_blob = new uint8_t[len];
		memcpy(_blob, file, len);
	}
	_bloblen = len;
	uint8_t *position = file + 3, *end = file + len;
	_type = tagCompound;
	NBT_Tag::parseData(position, end);
	if (position == NULL) {
		//fprintf(stderr, "Error parsing NBT from stream\n");
	}
	success = (position != NULL);
}

NBT::~NBT()
{
	if (_blob != NULL) {
		//fprintf(stderr, "DELETE BLOB\n");
		delete[] _blob;
	}
	if (_filename != NULL) {
		free(_filename);
	}
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

NBT_Tag::NBT_Tag()
{
	_list = NULL;
	_elems = NULL;
	_data = NULL;
	_type = tagUnknown;
}

NBT_Tag::NBT_Tag(uint8_t* &position, const uint8_t *end, string &name)
{
	_list = NULL;
	_elems = NULL;
	_data = NULL;
	_type = tagUnknown;
	if (*position < 1 || *position > 11 || end - position < 3) {
		position = NULL;
		return;
	}
	_type = (TagType)*position;
	uint16_t len = _ntohs(position+1);
	if (end - position < len) {
		position = NULL;
		return;
	}
	name = string((char *)position+3, len);
	position += 3 + len;
	//fprintf(stderr, "Tag name is %s\n", name.c_str());
	this->parseData(position, end, &name);
}

NBT_Tag::NBT_Tag(uint8_t* &position, const uint8_t *end, TagType type)
{
	// this contructor is used for TAG_List entries
	_list = NULL;
	_elems = NULL;
	_data = NULL;
	_type = type;
	this->parseData(position, end);
}

void NBT_Tag::parseData(uint8_t* &position, const uint8_t *end, string *name)
{
	// position should now point to start of data (for named tags, right after the name)
	//fprintf(stderr, "Have Tag of type %d\n", (int)_type);
	switch (_type) {
	case tagCompound:
		_elems = new tagmap;
		while (*position != 0 && position < end) { // No end tag, go on...
			//fprintf(stderr, "Having a child....*plopp*..\n");
			string thisname;
			NBT_Tag *tmp = new NBT_Tag(position, end, thisname);
			if (position == NULL) {
				//fprintf(stderr, "DELETE tmp in compound because of invalid\n");
				delete tmp;
				return;
			}
			if (name != NULL) {
				//fprintf(stderr, " ** Adding %s to %s\n", thisname.c_str(), name->c_str());
			}
			(*_elems)[thisname] = tmp;
		}
		++position;
		break;
	case tagList: {
		if (*position < 0 || *position > 11) {
			//fprintf(stderr, "Invalid list type!\n");
			position = NULL;
			return;
		}
		TagType type = (TagType)*position;
		uint32_t count = _ntohl(position+1);
		position += 5;
		_list = new list<NBT_Tag *>;
		//fprintf(stderr, "List contains %d elements of type %d\n", (int)count, (int)type);
		while (count-- && position < end) { // No end tag, go on...
			NBT_Tag *tmp = new NBT_Tag(position, end, type);
			if (position == NULL) {
				//fprintf(stderr, "DELETE tmp in list because of invalid\n");
				delete tmp;
				return;
			}
			_list->push_back(tmp);
		}
	}
	break;
	case tagByte:
		_data = position;
		position += 1;
		break;
	case tagShort:
		_data = position;
		position += 2;
		break;
	case tagInt:
		_data = position;
		position += 4;
		break;
	case tagLong:
		_data = position;
		position += 8;
		break;
	case tagFloat:
		_data = position;
		position += 4;
		break;
	case tagDouble:
		_data = position;
		position += 8;
		break;
	case tagByteArray:
		_len = _ntohl(position);
		//fprintf(stderr, "Array size is %d\n", (int)_len);
		if (position + _len + 4 >= end) {
			printf("ByteArray too long by %d bytes!\n", int((position + _len + 4) - end));
			position = NULL;
			return;
		}
		_data = position + 4;
		position += 4 + _len;
		break;
	case tagString:
		_len = _ntohs(position);
		//fprintf(stderr, "Stringlen is %d\n", (int)_len);
		if (position + _len + 2 >= end) {
			//fprintf(stderr, "Too long!\n");
			position = NULL;
			return;
		}
		_data = position;
		position += 2 + _len;
		break;
	case tagIntArray:
		_len = _ntohl(position) * 4;
		//fprintf(stderr, "Array size is %d\n", (int)_len);
		if (position + _len + 4 >= end) {
			printf("IntArray too long by %d bytes!\n", int((position + _len + 4) - end));
			position = NULL;
			return;
		}
		_data = position + 4;
		position += 4 + _len;
		break;
	case tagUnknown:
	default:
		printf("UNKNOWN TAG_ %d!\n", (int)_type);
		position = NULL;
		break;
	}
}

NBT_Tag::~NBT_Tag()
{
	if (_elems) {
		for (tagmap::iterator it = _elems->begin(); it != _elems->end(); it++) {
			////fprintf(stderr, "_elems Deleting %p\n", (it->second));
			delete (it->second);
		}
		delete _elems;
	}
	if (_list) {
		for (list<NBT_Tag *>::iterator it = _list->begin(); it != _list->end(); it++) {
			////fprintf(stderr, "_list Deleting %p\n", *it);
			delete *it;
		}
		delete _list;
	}
}

void NBT_Tag::printTags()
{
	if (_type != tagCompound) {
		printf("Not a tagCompound\n");
		return;
	}
	for (tagmap::iterator it = _elems->begin(); it != _elems->end(); it++) {
		printf("Have compound '%s' of type %d\n", it->first.c_str(), (int)it->second->getType());
	}
	printf("End list.\n");
}


// ----- Data getters -------
// The return value is always a boolean, telling you if the tag was found and the operation
// was successful. getByteArray also returns the length of the array in its third parameter.
// Protip: Always perform a check if the array really has at least the size you expect it
// to have, to save you some headache.

bool NBT_Tag::getCompound(const string name, NBT_Tag* &compound)
{
	if (_type != tagCompound || _elems->find(name) == _elems->end() || (*_elems)[name]->getType() != tagCompound) {
		return false;
	}
	compound = (*_elems)[name];
	return true;
}

bool NBT_Tag::getList(const string name, list<NBT_Tag *>* &lst)
{
	if (_type != tagCompound || _elems->find(name) == _elems->end() || (*_elems)[name]->getType() != tagList) {
		return false;
	}
	lst = (*_elems)[name]->_list;
	return true;
}

bool NBT_Tag::getByte(const string name, int8_t &value)
{
	if (_type != tagCompound || _elems->find(name) == _elems->end() || (*_elems)[name]->getType() != tagByte) {
		return false;
	}
	value = *(int8_t*)(*_elems)[name]->_data;
	return true;
}

bool NBT_Tag::getShort(const string name, int16_t &value)
{
	if (_type != tagCompound || _elems->find(name) == _elems->end() || (*_elems)[name]->getType() != tagShort) {
		return false;
	}
	value = _ntohs((*_elems)[name]->_data);
	return true;
}

bool NBT_Tag::getInt(const string name, int32_t &value)
{
	if (_type != tagCompound || _elems->find(name) == _elems->end() || (*_elems)[name]->getType() != tagInt) {
		return false;
	}
	value = _ntohl((*_elems)[name]->_data);
	return true;
}

bool NBT_Tag::getLong(const string name, int64_t &value)
{
	if (_type != tagCompound || _elems->find(name) == _elems->end() || (*_elems)[name]->getType() != tagLong) {
		return false;
	}
	unsigned char *data = (unsigned char *)(*_elems)[name]->_data;
	value = int64_t(
	           ((uint64_t)data[0] << 56)
	           + ((uint64_t)data[1] << 48)
	           + ((uint64_t)data[2] << 40)
	           + ((uint64_t)data[3] << 32)
	           + ((uint64_t)data[4] << 24)
	           + ((uint64_t)data[5] << 16)
	           + ((uint64_t)data[6] << 8)
	           + ((uint64_t)data[8])); // Looks like crap, but should be endian-safe
	return true;
}

bool NBT_Tag::getByteArray(const string name, uint8_t* &data, int &len)
{
	if (_type != tagCompound || _elems->find(name) == _elems->end() || (*_elems)[name]->getType() != tagByteArray) {
		return false;
	}
	data = (*_elems)[name]->_data;
	len = (*_elems)[name]->_len;
	return true;
}
