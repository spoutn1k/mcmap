/*! \file nbt.h
 * \brief Definition of NBT and NBT_Tag classes */

#ifndef _NBT_H_
#define _NBT_H_

#include <map>
#include <list>
#include <string>
#include "json.hpp"
#include "helper.h"

using std::string;
using std::map;
using std::list;
using json = nlohmann::json;

class NBT_Tag;
typedef map<string, NBT_Tag*> nbt_map;
typedef list<NBT_Tag*> nbt_list;

enum TagType {
	T_UNKNOWN = 0, // Tag_End is not made available, so 0 should never be in any list of available elements
	T_BYTE = 1,
	T_SHORT = 2,
	T_INT = 3,
	T_LONG = 4,
	T_FLOAT = 5,
	T_DOUBLE = 6,
	T_BYTEARRAY = 7,
	T_STRING = 8,
	T_LIST = 9,
	T_COMPOUND = 10,
	T_INTARRAY = 11,
	T_LONGARRAY = 12
};

class NBT_Tag {
	friend class NBT;

	private:
	public:
	/*! \brief The tag's name */
	string _name;

	/*! \brief The tag's name length */
	uint16_t _name_length;

	/*! \brief The tag's type. */
	TagType _type;

	/*! \brief Map containing a compound's tags */
	nbt_map _compound_content;

	/*! \brief List containing a list's tags */
	nbt_list _list_content;

	/*! \brief Buffer for imported data.
	 *
	 * Contains the relevant information from the read file */
	uint8_t *_data;

	/*! \brief Length of the `_data` buffer */
	uint32_t _len;

	bool parseData(uint8_t* &position, const uint8_t *end);

	NBT_Tag(const NBT_Tag& to_copy);
	NBT_Tag(NBT_Tag&& to_move);

	explicit NBT_Tag(uint8_t* &position, const uint8_t *end);
	explicit NBT_Tag(uint8_t* &position, const uint8_t *end, TagType type);
	explicit NBT_Tag();

	virtual ~NBT_Tag();
	json toJson() const;

	TagType getType() const {return _type;}
	string getName() const {return _name;}

	bool getByte(int8_t &value) const;
	bool getShort(int16_t &value) const;
	bool getInt(int32_t &value) const;
	bool getLong(int64_t &value) const;
	bool getFloat(float &value) const;
	bool getDouble(double &value) const;
	bool getByteArray(uint8_t* &data, uint32_t &len) const;
	bool getString(string &value) const;
	bool getList(nbt_list &value) const;
	bool getCompound(nbt_map &value) const;
	bool getIntArray(uint8_t* &data, uint32_t &len) const;
	bool getLongArray(uint8_t* &data, uint32_t &len) const;

	nbt_list getList() const {
		return _list_content;
	}

	int8_t getByte() const {
		int8_t value;
		getByte(value);
		return value;
	}
	int16_t getShort() const {
		int16_t value;
		getShort(value);
		return value;
	}
	int32_t getInt() const {
		int32_t value;
		getInt(value);
		return value;
	}
	int64_t getLong() const {
		int64_t value;
		getLong(value);
		return value;
	}
	float getFloat() const {
		float value;
		getFloat(value);
		return value;
	}
	double getDouble() const {
		double value;
		getDouble(value);
		return value;
	}
	list<uint8_t> getByteArray() const {
		uint8_t* data;
		uint32_t len;
		list<uint8_t> out;
		getByteArray(data, len);
		for (uint32_t i = 0; i < len; i++)
			out.push_back(data[i]);
		return out;
	}
	string getString() const {
		string value;
		getString(value);
		return value;
	}
	nbt_map getCompound() const {
		nbt_map value;
		getCompound(value);
		return value;
	}
	list<int32_t> getIntArray() const {
		uint8_t* data;
		uint32_t len;
		list<int32_t> out;
		getIntArray(data, len);
		for (uint32_t i = 0; i < len; i+=4)
			out.push_back(_ntohl(&data[i]));
		return out;
	}
	list<int64_t> getLongArray() const {
		uint8_t* data;
		uint32_t len;
		list<int64_t> out;
		getLongArray(data, len);
		for (uint32_t i = 0; i < len; i+=8)
			out.push_back(_ntohll(&data[i]));
		return out;
	}

	// This is the original getter template, which I got rid of in favour
	// of individual getters by tag type. 
	/*bool getByteChild(const string name, int8_t &value) {
	  if (_type != T_COMPOUND || _compound_content->find(name) == _compound_content->end() || (*_compound_content)[name]->getType() != T_BYTE) {
	  return false;
	  }
	  return (*_compound_content)[name]->getByte(value);
	  }*/

	NBT_Tag operator[](string child_name);
	NBT_Tag operator[](uint64_t index);

	// Clone operator
	NBT_Tag operator=(const NBT_Tag& original);
	NBT_Tag operator=(NBT_Tag&& original);
};

class NBT : public NBT_Tag {
	private:
		/* \brief The name of the opened file */
		char* _filename;
		uint8_t* _blob;
		uint32_t _bloblen;

	public:
		explicit NBT(uint8_t * const data, const size_t len, bool &success);
		explicit NBT(const char *file, bool &success);
		virtual ~NBT();
		//bool save();
};


string getBlockId(uint64_t position, NBT_Tag* section);
#endif
