#ifndef FTS_INDEX_H
#define FTS_INDEX_H

/**
 *  Utilities for getting and managing metadata for full-text indexes.
 *
 *  The original code was created by Simonov Denis
 *  for the open source Lucene UDR full-text search library for Firebird DBMS.
 *
 *  Copyright (c) 2022 Simonov Denis <sim-mail@list.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
**/

#include "LuceneUdr.h"
#include "Relations.h"
#include "LuceneHeaders.h"
#include "FileUtils.h"
#include <string>
#include <list>
#include <map>
#include <memory>
#include <algorithm>

using namespace Firebird;
using namespace std;
using namespace Lucene;

namespace LuceneUDR
{
	/// <summary>
	/// Metadata for a full-text index segment.
	/// </summary>
	class FTSIndexSegment
	{
	public:
		string indexName;
		string fieldName;
		bool key;
		double boost;
		bool boostNull;
		bool fieldExists;

		FTSIndexSegment()
			: indexName()
			, fieldName()
			, key(false)
			, boost(1.0)
			, boostNull(true)
			, fieldExists(false)
		{}

		bool compareFieldName(const string& aFieldName) {
			return (fieldName == aFieldName) || (fieldName == "RDB$DB_KEY" && aFieldName == "DB_KEY");
		}
	};

	using FTSIndexSegmentPtr = unique_ptr<FTSIndexSegment>;
	using FTSIndexSegmentList = list<FTSIndexSegmentPtr>;
	using FTSIndexSegmentsMap = map<string, FTSIndexSegmentList>;


	/// <summary>
	/// Full-text index metadata.
	/// </summary>
	class FTSIndex
	{
	public: 

		FTSIndex()
			: indexName()
			, relationName()
			, analyzer()
			, description()
			, status()
			, segments()
		{}

		string indexName;
		string relationName;
		string analyzer;		
		string description;
		string status; // N - new index, I - inactive, U - need rebuild, C - complete

		FTSIndexSegmentList segments;

		string sqlExractRecord;

		bool isActive() {
			return (status == "C") || (status == "U");
		}

		FTSIndexSegmentList::const_iterator findSegment(const string& fieldName) {
			return std::find_if(
				segments.cbegin(),
				segments.cend(),
				[&fieldName](const auto& segment) { return segment->compareFieldName(fieldName); }
			);
		}

		FTSIndexSegmentList::const_iterator findKey() {
			return std::find_if(
				segments.cbegin(),
				segments.cend(),
				[](const auto& segment) { return segment->key; }
			);
		}

		bool checkAllFieldsExists()
		{
			bool existsFlag = true;
			for (const auto& segment : segments) {
				existsFlag = existsFlag && segment->fieldExists;
			}
			return existsFlag;
		}

		string buildSqlSelectFieldValues(
			ThrowStatusWrapper* status,
			const unsigned int sqlDialect,
			const bool whereKey = false);
	};

	using FTSIndexPtr = unique_ptr<FTSIndex>;
	using FTSIndexList = list<FTSIndexPtr>;
	using FTSIndexMap = map<string, FTSIndexPtr>;

	/// <summary>
	/// Returns the directory where full-text indexes are located.
	/// </summary>
	/// 
	/// <param name="context">The context of the external routine.</param>
	/// 
	/// <returns>Full path to full-text index directory</returns>
	string getFtsDirectory(IExternalContext* context);

	inline bool createIndexDirectory(const string &indexDir)
	{
		auto indexDirUnicode = StringUtils::toUnicode(indexDir);
		if (!FileUtils::isDirectory(indexDirUnicode)) {
			return FileUtils::createDirectory(indexDirUnicode);
		}
		return true;
	}

	inline bool createIndexDirectory(const String &indexDir)
	{
		if (!FileUtils::isDirectory(indexDir)) {
			return FileUtils::createDirectory(indexDir);
		}
		return true;
	}

	inline bool removeIndexDirectory(const string &indexDir)
	{
		auto indexDirUnicode = StringUtils::toUnicode(indexDir);
		if (FileUtils::isDirectory(indexDirUnicode)) {
			return FileUtils::removeDirectory(indexDirUnicode);
		}
		return true;
	}

	inline bool removeIndexDirectory(const String &indexDir)
	{
		if (FileUtils::isDirectory(indexDir)) {
			return FileUtils::removeDirectory(indexDir);
		}
		return true;
	}



	/// <summary>
	/// Repository of full-text indexes. 
	/// 
	/// Allows you to retrieve and manage full-text index metadata.
	/// </summary>
	class FTSIndexRepository final
	{

	private:
		IMaster* m_master;	
		// prepared statements
		AutoRelease<IStatement> stmt_exists_index;
		AutoRelease<IStatement> stmt_get_index;
		AutoRelease<IStatement> stmt_index_segments;
		AutoRelease<IStatement> stmt_rel_segments;
		AutoRelease<IStatement> stmt_key_segment;
	public:
		RelationHelper relationHelper;

		FTSIndexRepository()
			: FTSIndexRepository(nullptr)
		{}

		FTSIndexRepository(IMaster* master)
			: m_master(master)
			, relationHelper(master)
			, stmt_exists_index(nullptr)
			, stmt_get_index(nullptr)
			, stmt_index_segments(nullptr)
			, stmt_rel_segments(nullptr)
			, stmt_key_segment(nullptr)
		{
		}


		/// <summary>
		/// Create a new full-text index. 
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// <param name="relationName">Relation name</param>
		/// <param name="analyzerName">Analyzer name</param>
		/// <param name="description">Custom index description</param>
		void createIndex (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName,
			const string& relationName,
			const string& analyzerName,
			const string& description);

		/// <summary>
		/// Remove a full-text index. 
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		void dropIndex (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName);

		/// <summary>
		/// Set the index status.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// <param name="indexStatus">Index Status</param>
		void setIndexStatus (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName,
			const string& indexStatus);

		/// <summary>
		/// Checks if an index with the given name exists.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// 
		/// <returns>Returns true if the index exists, false otherwise</returns>
		bool hasIndex (
			ThrowStatusWrapper* status, 
			IAttachment* att, 
			ITransaction* tra, 
			const unsigned int sqlDialect, 
			const string& indexName);

		/// <summary>
		/// Returns index metadata by index name.
		/// 
		/// Throws an exception if the index does not exist. 
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// <param name="withSegments">Fill segments list</param>
		/// 
		/// <returns>Index metadata</returns>
		FTSIndexPtr getIndex (
			ThrowStatusWrapper* status, 
			IAttachment* att, 
			ITransaction* tra, 
			const unsigned int sqlDialect, 
			const string& indexName,
			const bool withSegments = false);

		/// <summary>
		/// Returns a list of indexes. 
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// 
		/// <returns>List of indexes</returns>
		FTSIndexList getAllIndexes (
			ThrowStatusWrapper* status, 
			IAttachment* att, 
			ITransaction* tra, 
			const unsigned int sqlDialect);

		/// <summary>
		/// Returns a list of indexes with segments. 
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// 
		/// <returns>Map indexes of name with segments</returns>
		FTSIndexMap getAllIndexesWithSegments(
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect);

		/// <summary>
		/// Returns a list of index segments with the given name.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// <param name="segments">Segments list</param>
		/// 
		/// <returns>List of index segments</returns>
		void fillIndexSegments(
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName,
			FTSIndexSegmentList& segments);



		/// <summary>
		/// Checks if an index key field exists for given relation.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// 
		/// <returns>Returns true if the index field exists, false otherwise</returns>
		bool hasKeyIndexField(
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName
		);

		/// <summary>
		/// Returns segment with key field.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// 
		/// <returns>Returns segment with key field.</returns>
		FTSIndexSegmentPtr getKeyIndexField (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName
		);

		/// <summary>
		/// Adds a new field (segment) to the full-text index.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// <param name="fieldName">Field name</param>
		/// <param name="key">Field is key</param>
		/// <param name="boost">Significance multiplier</param>
		/// <param name="boostNull">Boost null flag</param>
		void addIndexField (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName,
			const string& fieldName,
			const bool key,
			const double boost = 1.0,
			const bool boostNull = true);

		/// <summary>
		/// Removes a field (segment) from the full-text index.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// <param name="fieldName">Field name</param>
		void dropIndexField (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName,
			const string& fieldName);

		/// <summary>
		/// Sets the significance multiplier for the index field.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// <param name="fieldName">Field name</param>
		/// <param name="boost">Significance multiplier</param>
		/// <param name="boostNull">Boost null flag</param>
		void setIndexFieldBoost(
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName,
			const string& fieldName,
			const double boost,
			const bool boostNull = false);


		/// <summary>
		/// Checks for the existence of a field (segment) in a full-text index. 
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="indexName">Index name</param>
		/// <param name="fieldName">Field name</param>
		/// <returns>Returns true if the field (segment) exists in the index, false otherwise</returns>
		bool hasIndexSegment (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& indexName,
			const string& fieldName);


		/// <summary>
		/// Returns a list of full-text index field names given the relation name.
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="relationName">Relation name</param>
		/// 
		/// <returns>List of full-text index field names</returns>
		list<string> getFieldsByRelation (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string &relationName);


		/// <summary>
		/// Returns a list of trigger source codes to support full-text indexes by relation name. 
		/// </summary>
		/// 
		/// <param name="status">Firebird status</param>
		/// <param name="att">Firebird attachment</param>
		/// <param name="tra">Firebird transaction</param>
		/// <param name="sqlDialect">SQL dialect</param>
		/// <param name="relationName">Relation name</param>
		/// <param name="multiAction">Flag for generating multi-event triggers</param>
		/// 
		/// <returns>Trigger source code list</returns>
		list<string> makeTriggerSourceByRelation (
			ThrowStatusWrapper* status,
			IAttachment* att,
			ITransaction* tra,
			const unsigned int sqlDialect,
			const string& relationName,
			const bool multiAction);
	};
}
#endif	// FTS_INDEX_H
