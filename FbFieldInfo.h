#ifndef FB_FIELD_INFO_H
#define FB_FIELD_INFO_H

#include <string>
#include <vector>
#include "FBBlobUtils.h"
#include "FBAutoPtr.h"
#include "firebird/UdrCppEngine.h"

using namespace std;
using namespace Firebird;

template <typename T>
T as(unsigned char* ptr)
{
	return *((T*)ptr);
}

template <typename T>
inline string ToString(T tX)
{
	std::ostringstream oStream;
	oStream << tX;
	return oStream.str();
}

struct FbFieldInfo {
	string fieldName;
	string relationName;
	string owner;
	string alias;
	bool nullable;
	unsigned dataType;
	int subType;
	unsigned length;
	int scale;
	unsigned charSet;
	unsigned offset;
	unsigned nullOffset;

	inline bool isNull(unsigned char* buffer) {
		return as<short>(buffer + nullOffset);
	}

	inline FB_BOOLEAN getBooleanValue(unsigned char* buffer) {
		return as<FB_BOOLEAN>(buffer + offset);
	}

	inline ISC_SHORT getShortValue(unsigned char* buffer) {
		return as<ISC_SHORT>(buffer + offset);
	}

	inline ISC_LONG getLongValue(unsigned char* buffer) {
		return as<ISC_LONG>(buffer + offset);
	}

	inline ISC_INT64 getInt64Value(unsigned char* buffer) {
		return as<ISC_INT64>(buffer + offset);
	}

	inline FB_I128 getInt128Value(unsigned char* buffer) {
		return as<FB_I128>(buffer + offset);
	}

	inline float getFloatValue(unsigned char* buffer) {
		return as<float>(buffer + offset);
	}

	inline double getDoubleValue(unsigned char* buffer) {
		return as<double>(buffer + offset);
	}

	inline FB_DEC16 getDecFloat16Value(unsigned char* buffer) {
		return as<FB_DEC16>(buffer + offset);
	}

	inline FB_DEC34 getDecFloat34Value(unsigned char* buffer) {
		return as<FB_DEC34>(buffer + offset);
	}

	inline ISC_DATE getDateValue(unsigned char* buffer) {
		return as<ISC_DATE>(buffer + offset);
	}

	inline ISC_TIME getTimeValue(unsigned char* buffer) {
		return as<ISC_TIME>(buffer + offset);
	}

	inline ISC_TIME_TZ getTimeTzValue(unsigned char* buffer) {
		return as<ISC_TIME_TZ>(buffer + offset);
	}

	inline ISC_TIME_TZ_EX getTimeTzExValue(unsigned char* buffer) {
		return as<ISC_TIME_TZ_EX>(buffer + offset);
	}

	inline ISC_TIMESTAMP getTimestampValue(unsigned char* buffer) {
		return as<ISC_TIMESTAMP>(buffer + offset);
	}

	inline ISC_TIMESTAMP_TZ getTimestampTzValue(unsigned char* buffer) {
		return as<ISC_TIMESTAMP_TZ>(buffer + offset);
	}

	inline ISC_TIMESTAMP_TZ_EX getTimestampTzExValue(unsigned char* buffer) {
		return as<ISC_TIMESTAMP_TZ_EX>(buffer + offset);
	}

	inline ISC_QUAD getQuadValue(unsigned char* buffer) {
		return as<ISC_QUAD>(buffer + offset);
	}

	inline short getOctetsLength(unsigned char* buffer) {
		switch (dataType)
		{
		case SQL_TEXT:
			return length;
		case SQL_VARYING:
			return as<short>(buffer + offset);
		default:
			return 0;
		}
	}

	inline char* getCharValue(unsigned char* buffer) {
		switch (dataType)
		{
		case SQL_TEXT:
			return (char*)(buffer + offset);
		case SQL_VARYING:
			return (char*)(buffer + offset + sizeof(short));
		default:
			return nullptr;
		}
	}

	template <class StatusType> string getStringValue(StatusType* status, IAttachment* att, ITransaction* tra, unsigned char* buffer);
};

template <class StatusType> string FbFieldInfo::getStringValue(StatusType* status, IAttachment* att, ITransaction* tra, unsigned char* buffer)
{
	switch (dataType) {
	case SQL_TEXT:
	case SQL_VARYING:
	{
		string s(getCharValue(buffer), getOctetsLength(buffer));
		return s;
	}
	case SQL_BLOB:
	{
		ISC_QUAD blobId = getQuadValue(buffer);
		AutoRelease<IBlob> blob(att->openBlob(status, tra, &blobId, 0, nullptr));
		string s = blob_get_string(status, blob);
		blob->close(status);
		return s;
	}
	default:
		// ��������� ���� ���� �� �������������
		return "";
	}
}

template <class StatusType>  vector<FbFieldInfo> getFieldsInfo(StatusType* status, IMessageMetadata* meta)
{
	int colCount = meta->getCount(status);
	vector<FbFieldInfo> fields(colCount);
	for (unsigned i = 0; i < colCount; i++) { 
		FbFieldInfo field;
		field.nullable = meta->isNullable(status, i);
		field.fieldName.assign(meta->getField(status, i));
		field.relationName.assign(meta->getRelation(status, i));
		field.owner.assign(meta->getOwner(status, i));
		field.alias.assign(meta->getAlias(status, i));
		field.dataType = meta->getType(status, i);
		field.subType = meta->getSubType(status, i);
		field.length = meta->getLength(status, i);
		field.scale = meta->getScale(status, i);
		field.charSet = meta->getCharSet(status, i);
		field.offset = meta->getOffset(status, i);
		field.nullOffset = meta->getNullOffset(status, i);
		fields[i] = field;
	}
	return fields;
}

#endif	// FB_AUTO_PTR_H