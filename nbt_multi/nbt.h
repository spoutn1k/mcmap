#ifndef _NBT_H_
#define _NBT_H_
#include <cstring>
#include <map>
#include <list>
#include <vector>
#include <string>
#include "helper.h"

using std::string;
using std::map;
using std::vector;
using std::list;

class NBT_Tag;
typedef list<NBT_Tag *> taglist;
typedef vector<NBT_Tag *> tagvector;
typedef map<string, tagvector *> tagmap;

enum TagType {
	tagUnknown = 0, // Tag_End is not made available, so 0 should never be in any list of available elements
	tagByte = 1,
	tagShort = 2,
	tagInt = 3,
	tagLong = 4,
	tagFloat = 5,
	tagDouble = 6,
	tagByteArray = 7,
	tagString = 8,
	tagList = 9,
	tagCompound = 10,
	tagIntArray = 11
};

class NBT_Tag
{
	friend class NBT;
private:
	tagmap *_elems;
	taglist *_list;
	TagType _type;
	uint8_t *_data;
	uint32_t _len;

	explicit NBT_Tag(uint8_t* &position, const uint8_t *end, string &name);
	explicit NBT_Tag(uint8_t* &position, const uint8_t *end, TagType type);
	explicit NBT_Tag();
	void parseData(uint8_t* &position, const uint8_t *end, string *name = NULL);
	NBT_Tag* getAs(const string &name, const size_t index);

public:
	bool getCompound(const string name, NBT_Tag* &compound, int index = 0);
	bool getShort(const string name, int16_t &value, int index = 0);
	bool getInt(const string name, int32_t &value, int index = 0);
	bool getLong(const string name, int64_t &value, int index = 0);
	bool getByteArray(const string name, uint8_t* &data, int &len, int index = 0);
	TagType getType() {
		return _type;
	}
	virtual ~NBT_Tag();
};

class NBT : public NBT_Tag
{
private:
	uint8_t *_blob;
	char *_filename;
	size_t _bloblen;
public:
	explicit NBT(const char *file, bool &success);
	explicit NBT(uint8_t * const file, const size_t len, const bool shared, bool &success);
	explicit NBT();
	virtual ~NBT();
	//bool save();
};

#endif
