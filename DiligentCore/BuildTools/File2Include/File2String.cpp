// File2Include.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if( argc < 3 )
    {
        printf( "Incorrect number of command line arguments. Expected arguments: src file, dst file\n");
        return -1;
    }
    auto SrcFile = argv[1];
    auto DstFile = argv[2];
    if (strcmp(SrcFile, DstFile) == 0)
    {
        printf( "Source and destination files must be different\n");
        return -1;
    }

    FILE *pSrcFile = fopen( SrcFile, "r" );
    if( pSrcFile == nullptr )
    {
        printf( "Failed to open source file %s\n", SrcFile );
        return -1;
    }

    FILE *pDstFile = fopen( DstFile, "w" );
    if( pDstFile == nullptr )
    {
        printf( "Failed to open destination file %s\n", DstFile );
        fclose(pSrcFile);
        return -1;
    }


    char Buff[2048];
    char SpecialChars[] = "\'\"\\";
    while( !feof( pSrcFile ) )
    {
        auto* Line = fgets( Buff, sizeof( Buff )/sizeof(Buff[0]) , pSrcFile );
        if( Line == nullptr )
            break;
        fputc( '\"', pDstFile );
        auto* CurrChar = Line;
        while( *CurrChar != 0 && *CurrChar != '\n' && *CurrChar != '\r' )
        {
            if( strchr( SpecialChars, *CurrChar) )
                fputc( '\\', pDstFile );
            fputc( *CurrChar, pDstFile );
            ++CurrChar;
        }
        fputs( "\\n\"\n", pDstFile );
    }

    fclose(pDstFile);
    fclose(pSrcFile);

    printf( "File2String: sucessfully converted %s to %s\n", SrcFile, DstFile );

	return 0;
}
