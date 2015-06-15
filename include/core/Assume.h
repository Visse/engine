#pragma once


namespace Core
{
    void AssumptionFailed( const char *expression, const char *file, int line, const char *function );
}


#define ASSUME_TRUE( expr )  (expr) ? (void)0 : ::Core::AssumptionFailed( #expr, __FILE__, __LINE__, __FUNCTION__ )
