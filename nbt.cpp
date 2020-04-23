#include <cstdio>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <zlib.h>
#include "nbt.h"

#define ID_SIZE 1
#define NAME_LENGTH_SIZE 2

using std::memcpy;

NBT::NBT(uint8_t * const data, const size_t len, bool &success) {
	uint8_t *position = data + ID_SIZE + NAME_LENGTH_SIZE, *end = data + len;
	_type = T_COMPOUND;

	if (!NBT_Tag::parseData(position, end)) {
		fprintf(stderr, "Error parsing the NBT data\n");
		success = false;
		return;
	}

	if (position == NULL) {
		fprintf(stderr, "Error reading the data\n");
		success = false;
		return;
	}

	success = (position != NULL);
}

NBT::NBT(const char *file, bool &success) {
	int ret;

	_blob = NULL;
	_filename = NULL;
	gzFile fh = 0;
	success = true;
	_name = string("");

	if (file == NULL || *file == '\0' || !fileExists(file)) {
		success = false;
		return;
	}

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

NBT::~NBT() {
	if (_bloblen) {
		delete[] _blob;
		_bloblen = 0;
	}
}

//  _   _ ____ _____ _____           
// | \ | | __ )_   _|_   _|_ _  __ _ 
// |  \| |  _ \ | |   | |/ _` |/ _` |
// | |\  | |_) || |   | | (_| | (_| |
// |_| \_|____/ |_|___|_|\__,_|\__, |
//				 |_____|       |___/ 

NBT_Tag::NBT_Tag() : _name(""), _compound_content{}, _list_content{} {
	_data = NULL;
	_type = T_UNKNOWN;
	_name_length = 0;
	_len = 0;
}

NBT_Tag::NBT_Tag(const NBT_Tag& to_copy) : NBT_Tag() { 
	printf("Copy constructor: %s\n", to_copy._name.c_str());
	_type = to_copy._type;
	_name = to_copy._name;

	if (_type == T_COMPOUND) {
		for (auto it : to_copy._compound_content) {
			_compound_content.insert_or_assign(it.first, new NBT_Tag(*it.second));
		}
	} else if (_type == T_LIST) {
		for (auto it : to_copy._list_content) {
			_list_content.push_back(new NBT_Tag(*it));
		}
	} else {
		_len = to_copy._len;
		_data = to_copy._data;
		//_data = new uint8_t[_len];
		//memcpy(_data, to_copy._data, _len);
		//if (*_data != *to_copy._data)
			//printf("Copy: %d - %d (%s)\n", *to_copy._data, *_data, _name.c_str());
	}
}

NBT_Tag::NBT_Tag(NBT_Tag &&to_move) : NBT_Tag() { 
	printf("Move constructor: %s\n", to_move._name.c_str());
	_type = to_move._type;
	_name = to_move._name;

	if (_type == T_COMPOUND) {
		for (auto it : to_move._compound_content) {
			_compound_content.insert_or_assign(it.first, new NBT_Tag(*it.second));
		}
	} else if (_type == T_LIST) {
		for (auto it : to_move._list_content) {
			_list_content.push_back(new NBT_Tag(*it));
		}
	} else {
		_len = to_move._len;
		_data = to_move._data;
	}
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

	_name = string((char *)(position + ID_SIZE + NAME_LENGTH_SIZE), _name_length);
	position += 3 + _name_length;

	if (!parseData(position, end)) {
		fprintf(stderr, "Error parsing the NBT data\n");
		return;
	}
}

NBT_Tag::NBT_Tag(uint8_t* &position, const uint8_t *end, TagType type) : NBT_Tag() {
	// this contructor is used for TAG_List entries, because they have no name
	_type = type;

	if (!parseData(position, end)) {
		fprintf(stderr, "Error parsing the NBT data\n");
		return;
	}
}

bool NBT_Tag::parseData(uint8_t* &position, const uint8_t *end) {
	// position should now point to start of data (for named tags, right after the name)
	uint32_t count;
	switch (_type) {
		case T_COMPOUND:
			while (*position != 0 && position < end) { // No end tag, go on...
				//NBT_Tag tmp = NBT_Tag(position, end);
				NBT_Tag* tmp = new NBT_Tag(position, end);
				if (position == NULL) {
					delete tmp;
					return false;
				}
				_compound_content[tmp->_name] = tmp;
				//_compound_content.insert_or_assign(tmp._name, tmp);
			}
			++position;
			break;

		case T_LIST:
			if (*position > 11) {
				position = NULL;
				return false;
			}
			TagType type;
			if (*position == 0)
				type = (TagType)1; // ((*position)+1);
			else
				type = (TagType)*position;
			count = _ntohl(position+1);
			// We skip the type and length
			position += 5;

			while (count-- && position < end) { // No end tag, go on...
				//NBT_Tag tmp = NBT_Tag(position, end, type);
				NBT_Tag* tmp = new NBT_Tag(position, end, type);
				if (position == NULL) {
					return false;
				}
				_list_content.push_back(tmp);
			}
			break;

		case T_BYTE:
			_len = 1;
			_data = new uint8_t[_len];
			memcpy(_data, position, _len);

			position += 1;
			break;

		case T_SHORT:
			_len = 2;
			_data = new uint8_t[_len];
			memcpy(_data, position, _len);

			position += 2;
			break;

		case T_INT:
			_len = 4;
			_data = new uint8_t[_len];
			memcpy(_data, position, _len);

			position += 4;
			break;

		case T_LONG:
			_len = 8;
			_data = new uint8_t[_len];
			memcpy(_data, position, _len);

			position += 8;
			break;

		case T_FLOAT:
			_len = 4;
			_data = new uint8_t[_len];
			memcpy(_data, position, _len);

			position += 4;
			break;

		case T_DOUBLE:
			_len = 8;
			_data = new uint8_t[_len];
			memcpy(_data, position, _len);

			position += 8;
			break;

		case T_BYTEARRAY:
			_len = _ntohl(position);
			if (position + _len + 4 >= end) {
				fprintf(stderr, "ByteArray too long by %d bytes!\n", int((position + _len + 4) - end));
				position = NULL;
				return false;
			}
			_data = new uint8_t[_len];
			memcpy(_data, position + 4, _len);

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
			_data = new uint8_t[_len];
			memcpy(_data, position + 2, _len);
			//_data[_len] = '\0';

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

				_data = new uint8_t[_len];
				memcpy(_data, position + header_size, _len);

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

				_data = new uint8_t[_len];
				memcpy(_data, position + header_size, _len);

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
	if (_data && (_type != T_COMPOUND 
				&& _type != T_LIST)) {
		//delete[] _data;
		//_data = NULL;
	} else if (_type == T_COMPOUND) {
		for (auto it : _compound_content) {
			delete it.second;
		}
	} else if (_type == T_LIST) {
		for (auto it : _list_content) {
			delete it;
		}
	}

}

bool NBT_Tag::getByte(int8_t &value) const {
	if (_type != T_BYTE) {
		fprintf(stderr, "Accessing byte of incompatible NBT_Tag\n");
		return 0;
	}
	value = *_data;
	return true;
}

bool NBT_Tag::getShort(int16_t &value) const {
	if (_type != T_SHORT) {
		fprintf(stderr, "Accessing short of incompatible NBT_Tag\n");
		return false;
	}

	value = _ntohs(_data);
	return true;
}

bool NBT_Tag::getInt(int32_t &value) const {
	if (_type != T_INT) {
		fprintf(stderr, "Accessing int of incompatible NBT_Tag\n");
		return false;
	}

	value = _ntohl(_data);
	return true;
}

bool NBT_Tag::getLong(int64_t &value) const {
	if (_type != T_LONG) {
		fprintf(stderr, "Accessing long of incompatible NBT_Tag\n");
		return false;
	}

	value = _ntohll(_data);
	return true;
}

bool NBT_Tag::getFloat(float &value) const {
	if (_type != T_FLOAT) {
		fprintf(stderr, "Accessing float of incompatible NBT_Tag\n");
		return false;
	}

	int32_t tmp = _ntohl(_data);
	value = *((float*) &tmp);
	return true;
}

bool NBT_Tag::getDouble(double &value) const {
	if (_type != T_DOUBLE) {
		fprintf(stderr, "Accessing double of incompatible NBT_Tag\n");
		return false;
	}

	int64_t tmp = _ntohll(_data);
	value = *((double*) &tmp);
	return true;
}

bool NBT_Tag::getByteArray(uint8_t* &data, uint32_t &len) const {
	if (_type != T_BYTEARRAY) {
		fprintf(stderr, "Accessing byte array of incompatible NBT_Tag\n");
		return false;
	}

	data = _data;
	len = _len;
	return true;
}

bool NBT_Tag::getString(string &value) const {
	if (_type != T_STRING) {
		fprintf(stderr, "Accessing string of incompatible NBT_Tag\n");
		return false;
	}

	value = string((char*) _data, _len);
	return true;
}

bool NBT_Tag::getList(nbt_list &value) const {
	if (_type != T_LIST) {
		fprintf(stderr, "Accessing list of incompatible NBT_Tag\n");
		return false;
	}

	value = _list_content;
	return true;
}

bool NBT_Tag::getCompound(nbt_map &compound) const {
	if (_type != T_COMPOUND) {
		fprintf(stderr, "Accessing compound map of incompatible NBT_Tag\n");
		return false;
	}

	compound = _compound_content;
	return true;
}

bool NBT_Tag::getIntArray(uint8_t* &data, uint32_t &len) const {
	if (_type != T_INTARRAY) {
		fprintf(stderr, "Accessing int array of incompatible NBT_Tag\n");
		return false;
	}

	data = _data;
	len = _len;
	return true;
}

bool NBT_Tag::getLongArray(uint8_t* &data, uint32_t &len) const {
	if (_type != T_LONGARRAY) {
		fprintf(stderr, "Accessing long array of incompatible NBT_Tag\n");
		return false;
	}

	data = _data;
	len = _len;
	return true;
}

NBT_Tag NBT_Tag::operator[](string child_name) {
	if (_type != T_COMPOUND || _compound_content.find(child_name) == _compound_content.end())
		throw std::invalid_argument(child_name);

	return NBT_Tag(*_compound_content[child_name]);
}

NBT_Tag NBT_Tag::operator[](uint64_t index) {
	if (_type != T_LIST || !(index < _list_content.size()))
		throw std::out_of_range("List index");

	auto it = _list_content.cbegin();
	for (uint64_t i = 0; i < index; i++)
		++it;
	return NBT_Tag(**it);
}

// Clone operator
NBT_Tag NBT_Tag::operator=(const NBT_Tag& to_copy) {
	printf("Copy assignment: %s\n", to_copy._name.c_str());
	_type = to_copy._type;
	_name = to_copy._name;

	if (_type == T_COMPOUND) {
		for (auto it : to_copy._compound_content) {
			_compound_content.insert_or_assign(it.first, new NBT_Tag(*it.second));
		}
	} else if (_type == T_LIST) {
		for (auto it : to_copy._list_content) {
			_list_content.push_back(new NBT_Tag(*it));
		}
	} else {
		_len = to_copy._len;
		_data = new uint8_t[_len];
		memcpy(_data, to_copy._data, _len);
	}

	return *this;
}

NBT_Tag NBT_Tag::operator=(NBT_Tag&& to_move) {
	printf("Move assignment: %s\n", to_move._name.c_str());
	_type = to_move._type;
	_name = to_move._name;

	if (_type == T_COMPOUND) {
		for (auto it : to_move._compound_content) {
			_compound_content.insert_or_assign(it.first, new NBT_Tag(*it.second));
		}
	} else if (_type == T_LIST) {
		for (auto it : to_move._list_content) {
			_list_content.push_back(new NBT_Tag(*it));
		}
	} else {
		_len = to_move._len;
		_data = to_move._data;
	}

	return *this;
}

json NBT_Tag::toJson() const {
	json out;

	switch (_type) {
		case T_COMPOUND:
			{
				json holder;
				out[getName()] = holder;

				for (auto it : _compound_content) {
					out[getName()].update(it.second->toJson());
				}

				break;
			}

		case T_BYTE:
			out[getName()] = getByte();
			break;
		case T_SHORT:
			out[getName()] = getShort();
			break;
		case T_INT:
			out[getName()] = getInt();
			break;
		case T_LONG:
			out[getName()] = getLong();
			break;
		case T_FLOAT:
			out[getName()] = getFloat();
			break;
		case T_DOUBLE:
			out[getName()] = getDouble();
			break;
		case T_STRING:
			out[getName()] = getString();
			break;
		case T_BYTEARRAY:
			out[getName()] = getByteArray();
			break;
		case T_LIST:
			{
				std::vector<json> array;
				for (auto it : _list_content) {
					array.push_back(it->toJson());
				}

				out[getName()] = array;
				break;
			}
		case T_INTARRAY:
			out[getName()] = getIntArray();
			break;
		case T_LONGARRAY:
			out[getName()] = getLongArray();
			break;
		case T_UNKNOWN:
			break;
	}

	return out;
}

string getBlockId(uint64_t position, NBT_Tag* section) {
	uint64_t size = (uint64_t) ceil(log2((*section)["Palette"].getList().size()));
	size = (4 > size ? 4 : size);
	uint8_t* raw_data = (*section)["BlockStates"]._data;
	uint64_t skip = position*size/64; // The number of longs to skip
	int64_t remain = position*size - 64*skip; // The bits to skip in the first non-skipped long
	int64_t overflow = remain + size - 64; // The bits in the first non-skipped long

	uint64_t mask = ((1l << size) - 1) << remain;
	uint64_t lower_data = (_ntohll(raw_data + skip*8) & mask) >> remain;

	if (overflow > 0) {
		uint64_t upper_data = _ntohll(raw_data + (skip+1)*8) & ((1l << overflow) -1);
		lower_data = lower_data | upper_data << (size - overflow);
	}

	return (*section)["Palette"][lower_data]["Name"].getString();
}
