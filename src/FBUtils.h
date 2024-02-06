#ifndef FB_BLOB_UTILS_H
#define FB_BLOB_UTILS_H

/**
 *  Various helper functions.
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


namespace LuceneUDR
{

    class BlobUtils final {
    public:
        static std::string getString(Firebird::ThrowStatusWrapper* status, Firebird::IBlob* blob);

        static void setString(Firebird::ThrowStatusWrapper* status, Firebird::IBlob* blob, const std::string& str);
    };


    const unsigned int getSqlDialect(Firebird::ThrowStatusWrapper* status, Firebird::IAttachment* att);

    /// <summary>
    /// Escapes the name of the metadata object depending on the SQL dialect. 
    /// </summary>
    /// 
    /// <param name="sqlDialect">SQL dialect</param>
    /// <param name="name">Metadata object name</param>
    /// 
    /// <returns>Returns the escaped name of the metadata object.</returns>
    inline std::string escapeMetaName(const unsigned int sqlDialect, const std::string& name)
    {
        if (name == "RDB$DB_KEY")
            return name;
        switch (sqlDialect) {
        case 1:
            return name;
        case 3:
        default:
            return "\"" + name + "\"";
        }
    }

    [[noreturn]]
    void throwException(Firebird::ThrowStatusWrapper* const status, const char* message, ...);

    struct IscRandomStatus {
    public:
        explicit IscRandomStatus(const std::string& message)
            : statusVector{ isc_arg_gds, isc_random,
                isc_arg_string, (ISC_STATUS)message.c_str(),
                isc_arg_end }
        {
        }

        explicit IscRandomStatus(const char* message)
            : statusVector{ isc_arg_gds, isc_random,
                isc_arg_string, (ISC_STATUS)message,
                isc_arg_end }
        {
        }

        explicit IscRandomStatus(const std::exception& e)
            : statusVector{ isc_arg_gds, isc_random,
                isc_arg_string, (ISC_STATUS)e.what(),
                isc_arg_end }
        {
        }

        operator const ISC_STATUS* () const { return statusVector; }

        static IscRandomStatus createFmtStatus(const char* message, ...);

    private:
        ISC_STATUS statusVector[5] = {};
    };

    Firebird::IMessageMetadata* prepareTextMetaData(Firebird::ThrowStatusWrapper* status, Firebird::IMessageMetadata* meta);
}

#endif	// FB_BLOB_UTILS_H