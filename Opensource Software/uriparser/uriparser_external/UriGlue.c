#include "UriGlue.h"

#include <string.h>
#include <stdio.h>

#include <uriparser/Uri.h>


bool URIParser_get_url(const char* absolute_uri, const char* relative_uri, char* buffer, uint32_t buffer_len)
{
    UriParserStateA stateA;
    const int defaultLen = 1024 * 8;
    uint32_t cpyLen = 0;
    char transformedUriText[defaultLen];

    /* param check */
    if ((NULL == absolute_uri) || (NULL == absolute_uri) || (NULL == buffer))
    {
				return false;    
    }   
    
    memset(transformedUriText, 0, defaultLen);

    /* Base */
    UriUriA baseUri;
    stateA.uri = &baseUri;
    int res = uriParseUriA(&stateA, absolute_uri);
    if (res != 0) 
    {
        uriFreeUriMembersA(&baseUri);
        return false;
    }

    /* Rel */
    UriUriA relUri;
    stateA.uri = &relUri;
    res = uriParseUriA(&stateA, relative_uri);
    if (res != 0) 
    {
        uriFreeUriMembersA(&baseUri);
        uriFreeUriMembersA(&relUri);
        return false;
    }

    /* Transform */
    UriUriA transformedUri;
    res = uriAddBaseUriA(&transformedUri, &relUri, &baseUri);
    if (res != 0) 
    {
        uriFreeUriMembersA(&baseUri);
        uriFreeUriMembersA(&relUri);
        uriFreeUriMembersA(&transformedUri);
        return false;
    }

    uriToStringA(transformedUriText, &transformedUri, defaultLen, NULL);

    uriFreeUriMembersA(&baseUri);
    uriFreeUriMembersA(&relUri);
    uriFreeUriMembersA(&transformedUri);
    
    cpyLen = (uint32_t)strlen(transformedUriText);
    if (cpyLen + 1 > buffer_len)
    {
    		return false;
    }
    
    memcpy(buffer, transformedUriText, cpyLen);
    buffer[cpyLen] = '\0';
    
    return true;
}