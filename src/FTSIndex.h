#ifndef FTS_INDEX_H
#define FTS_INDEX_H

/**
 *  Utilities for getting and managing metadata for full-text indexes.
 *
 *  The original code was created by Simonov Denis
 *  for the open source project "IBSurgeon Full Text Search UDR".
 *
 *  Copyright (c) 2022 Simonov Denis <sim-mail@list.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
**/

#include "LuceneUdr.h"

#include <string>
#include <list>
#include <unordered_set>
#include <map>
#include <memory>
#include <algorithm>

#include "FBFieldInfo.h"

namespace FTSMetadata
{

    FB_MESSAGE(FTSIndexNameInput, Firebird::ThrowStatusWrapper,
        (FB_INTL_VARCHAR(252, CS_UTF8), indexName)
    );

    FB_MESSAGE(FTSIndexRecord, Firebird::ThrowStatusWrapper,
        (FB_INTL_VARCHAR(252, CS_UTF8), indexName)
        (FB_INTL_VARCHAR(252, CS_UTF8), relationName)
        (FB_INTL_VARCHAR(252, CS_UTF8), analyzer)
        (FB_BLOB, description)
        (FB_INTL_VARCHAR(4, CS_UTF8), indexStatus)
    );

    enum class FTSKeyType {NONE, DB_KEY, INT_ID, UUID};

    class FTSIndexSegment;

    using FTSIndexSegmentPtr = std::unique_ptr<FTSIndexSegment>;
    using FTSIndexSegmentList = std::list<FTSIndexSegmentPtr>;
    using FTSIndexSegmentsMap = std::map<std::string, FTSIndexSegmentList>;


    class FTSPreparedIndexStmt final
    {
    public:
        FTSPreparedIndexStmt() = default;

        FTSPreparedIndexStmt(
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view sql
        );

        FTSPreparedIndexStmt(const FTSPreparedIndexStmt&) = delete;
        FTSPreparedIndexStmt& operator=(const FTSPreparedIndexStmt&) = delete;

        FTSPreparedIndexStmt(FTSPreparedIndexStmt&&) noexcept = default;
        FTSPreparedIndexStmt& operator=(FTSPreparedIndexStmt&&) noexcept = default;

        Firebird::IStatement* getPreparedExtractRecordStmt()
        {
            return m_stmtExtractRecord;
        }

        Firebird::IMessageMetadata* getOutExtractRecordMetadata()
        {
            return m_outMetaExtractRecord;
        }

        Firebird::IMessageMetadata* getInExtractRecordMetadata()
        {
            return m_inMetaExtractRecord;
        }
    private:
        Firebird::AutoRelease<Firebird::IStatement> m_stmtExtractRecord;
        Firebird::AutoRelease<Firebird::IMessageMetadata> m_inMetaExtractRecord;
        Firebird::AutoRelease<Firebird::IMessageMetadata> m_outMetaExtractRecord;
    };

    /// <summary>
    /// Full-text index metadata.
    /// </summary>
    class FTSIndex final
    {
    public:
        std::string indexName{ "" };
        std::string relationName{ "" };
        std::string analyzer{ "" };
        std::string status{ "" }; // N - new index, I - inactive, U - need rebuild, C - complete

        FTSIndexSegmentList segments;

        FTSKeyType keyFieldType{ FTSKeyType::NONE };
        std::wstring unicodeKeyFieldName{ L"" };
        std::wstring unicodeIndexDir{ L"" };
    public: 

        FTSIndex() = default;

        explicit FTSIndex(const FTSIndexRecord& record);

        bool isActive() const {
            return (status == "C") || (status == "U");
        }

        FTSIndexSegmentList::const_iterator findSegment(const std::string& fieldName);

        FTSIndexSegmentList::const_iterator findKey();

        bool checkAllFieldsExists();

        std::string buildSqlSelectFieldValues(
            Firebird::ThrowStatusWrapper* status,
            unsigned int sqlDialect,
            bool whereKey = false
        );
    };


    using FTSIndexPtr = std::unique_ptr<FTSIndex>;
    using FTSIndexList = std::list<FTSIndexPtr>;
    using FTSIndexMap = std::map<std::string, FTSIndexPtr>;

    /// <summary>
    /// Metadata for a full-text index segment.
    /// </summary>
    class FTSIndexSegment final
    {
    public:
        std::string indexName{""};
        std::string fieldName{""};
        bool key = false;
        double boost = 1.0;
        bool boostNull = true;
        bool fieldExists = false;
    public:
        FTSIndexSegment() = default;

        bool compareFieldName(const std::string& aFieldName) const {
            return (fieldName == aFieldName) || (fieldName == "RDB$DB_KEY" && aFieldName == "DB_KEY");
        }
    };


    class RelationHelper;
    class AnalyzerRepository;


    /// <summary>
    /// Repository of full-text indexes. 
    /// 
    /// Allows you to retrieve and manage full-text index metadata.
    /// </summary>
    class FTSIndexRepository final
    {

    private:
        Firebird::IMaster* m_master = nullptr;
        AnalyzerRepository* m_analyzerRepository{ nullptr };
        RelationHelper* m_relationHelper{nullptr};
        // prepared statements
        Firebird::AutoRelease<Firebird::IStatement> m_stmt_exists_index{ nullptr };
        Firebird::AutoRelease<Firebird::IStatement> m_stmt_get_index{ nullptr };
        Firebird::AutoRelease<Firebird::IStatement> m_stmt_index_fields{ nullptr };
        Firebird::AutoRelease<Firebird::IStatement> m_stmt_active_indexes_by_analyzer{ nullptr };

    public:

        FTSIndexRepository() = delete;


        explicit FTSIndexRepository(Firebird::IMaster* master);

        ~FTSIndexRepository();

        RelationHelper* getRelationHelper()
        {
            return m_relationHelper;
        }

        AnalyzerRepository* getAnalyzerRepository()
        {
            return m_analyzerRepository;
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
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName,
            std::string_view relationName,
            std::string_view analyzerName,
            ISC_QUAD* description);

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
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName);

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
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName,
            std::string_view indexStatus);

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
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect, 
            std::string_view indexName);

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
        FTSIndexPtr getIndex (
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect, 
            std::string_view indexName,
            bool withSegments = false);

        /// <summary>
        /// Returns a list of indexes. 
        /// </summary>
        /// 
        /// <param name="status">Firebird status</param>
        /// <param name="att">Firebird attachment</param>
        /// <param name="tra">Firebird transaction</param>
        /// <param name="sqlDialect">SQL dialect</param>
        /// <param name="indexes">List of indexes</param>
        /// 
        void fillAllIndexes (
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            FTSIndexList& indexes);

        /// <summary>
        /// Returns a list of indexes with fields (segments). 
        /// </summary>
        /// 
        /// <param name="status">Firebird status</param>
        /// <param name="att">Firebird attachment</param>
        /// <param name="tra">Firebird transaction</param>
        /// <param name="sqlDialect">SQL dialect</param>
        /// <param name="indexes">Map indexes of name with fields (segments)</param>
        /// 
        void fillAllIndexesWithFields(
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            FTSIndexMap& indexes);

        /// <summary>
        /// Returns a list of index fields with the given name.
        /// </summary>
        /// 
        /// <param name="status">Firebird status</param>
        /// <param name="att">Firebird attachment</param>
        /// <param name="tra">Firebird transaction</param>
        /// <param name="sqlDialect">SQL dialect</param>
        /// <param name="indexName">Index name</param>
        /// <param name="segments">Segments list</param>
        /// 
        /// <returns>List of index fields (segments)</returns>
        void fillIndexFields(
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName,
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
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName
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
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName,
            std::string_view fieldName,
            bool key,
            double boost = 1.0,
            bool boostNull = true);

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
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName,
            std::string_view fieldName);

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
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName,
            std::string_view fieldName,
            double boost,
            bool boostNull = false);


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
        bool hasIndexField (
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view indexName,
            std::string_view fieldName);


        bool hasIndexByAnalyzer(
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view analyzerName
        );

        std::unordered_set<std::string> getActiveIndexByAnalyzer(
            Firebird::ThrowStatusWrapper* status,
            Firebird::IAttachment* att,
            Firebird::ITransaction* tra,
            unsigned int sqlDialect,
            std::string_view analyzerName
        );
    };
    using FTSIndexRepositoryPtr = std::unique_ptr<FTSIndexRepository>;

}

#endif	// FTS_INDEX_H
