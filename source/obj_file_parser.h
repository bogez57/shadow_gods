#ifndef OBJ_FILE_PARSER_H
#define OBJ_FILE_PARSER_H

#include <stdlib.h>
#include <mpc/mpc.h>

struct ObjFileData
{
    RunTimeArr<v3> verts{};
    RunTimeArr<s16> indicies{};
};

void test_MPCLibrary();
void test_OBJFileParsing();

#endif //OBJ_FILE_PARSER_H


#ifdef OBJ_FILE_PARSER_IMPL

static void* read_line(void* line) {
    printf("Reading Line: %s", (char*)line);
    return line;
}

static mpc_val_t *print_token(mpc_val_t *x) {
    printf("Token: '%s'\n", (char*)x);
    
    return x;
};

enum Token_Type
{
    END_OF_FILE = 0,
    VECTOR = 'v',
    FACE = 'f'
};

struct Token
{
    Token_Type type;
    char* text{};
    s32 length{};
};

struct Tokenizer
{
    char* at;
};

bool IsWhiteSpace(char character)
{
    bool result = ((character == ' ' ||
                    character == '\t'||
                    character == '\n'||
                    character == '\r'));
    
    return result;
};

bool IsDigit(char character)
{
    bool result = ((character >= '0' && character <= '9'));
    
    return result;
};

bool IsPeriod(char character)
{
    bool result = (character == '.');
    
    return result;
};

bool IsMinusSign(char character)
{
    bool result = (character == '-');
    
    return result;
};

void AdvanceStream(char** stream)
{
    BGZ_ASSERT(stream != '\0', "Reached end of file!");
    ++*stream;
};

void AdvanceTokenizer(Tokenizer&& tokenizer)
{
    BGZ_ASSERT(*tokenizer.at != '\0', "Reached end of file!");
    ++tokenizer.at;
};

char Peek(char* string, s32 howFarAheadToPeek)
{
    BGZ_ASSERT(*string != '\0', "Reached end of file!");
    BGZ_ASSERT(string != nullptr, "Trying to read uninitialized pointer!");
    
    for(s32 i{}; i < howFarAheadToPeek; ++i)
    {
        ++string;
        if(*string == '\0')
            break;
    }
    
    char result = *string;
    
    return result;
};

/* date = June 10th 2020 4:41 am */

struct Vertex
{
    v3 position{};
    v2 texCoord{};
    v3 normal{};
};

RunTimeArr<Vertex> LoadOBJ(const char* filePath, Memory_Partition* memPart)
{
    RunTimeArr<v3> vertexPositions{memPart, 100};
    RunTimeArr<v2> vertexTexCoords{memPart, 100};
    RunTimeArr<v3> vertexNormals{memPart, 100};
    
    RunTimeArr<s32> vertexPositionIndicies{memPart, 100};
    RunTimeArr<s32> vertexTexCoordIndicies{memPart, 100};
    RunTimeArr<s32> vertexnormalIndicies{memPart, 100};
    
    RunTimeArr<Vertex> resultVertices{};
    
    s32 length{};
    char* fileContents = globalPlatformServices->ReadEntireFile($(length), filePath);
    char* stream = fileContents;
    
    bool contentsStillNeedParsed{true};
    while(contentsStillNeedParsed)
    {
        switch(stream[0])
        {
            case '\0':
            {
                contentsStillNeedParsed = false;
            }break;
            
            case 'v':
            {
                if(Peek(stream, 1) == 't')
                {
                    bool notAllTexCoordsParsed{true};
                    while(notAllTexCoordsParsed)
                    {
                        //Remove initial prefix and whitespace
                        AdvanceStream(&stream);
                        AdvanceStream(&stream);
                        AdvanceStream(&stream);
                        
                        while(IsDigit(stream[0]) || IsPeriod(stream[0]) || IsMinusSign(stream[0]))
                        {
                            v2 texCoord{};
                            for(s32 i{}; i < 2; ++i)
                            {
                                char scalarString[10]{};
                                s32 stringI{};
                                while(IsDigit(stream[0]) || IsPeriod(stream[0]) || IsMinusSign(stream[0]))
                                {
                                    scalarString[stringI] = *stream++;
                                    ++stringI;
                                };
                                
                                texCoord.elem[i] = (f32)strtod(scalarString, NULL);
                                AdvanceStream(&stream);
                            };
                            
                            vertexTexCoords.Push(texCoord);
                            
                            while(stream[0] != 'v')
                                AdvanceStream(&stream);
                        };
                        
                        BGZ_ASSERT(stream[0] != '\0', "Reached end of stream!");
                        
                        if(Peek(stream, 1) == 'n')
                            notAllTexCoordsParsed = false;
                    };
                }
                else if(Peek(stream, 1) == 'n')
                {
                    //normals
                    bool notAllNormalsParsed{true};
                    while(notAllNormalsParsed)
                    {
                        //Remove initial prefix and whitespace
                        AdvanceStream(&stream);
                        AdvanceStream(&stream);
                        AdvanceStream(&stream);
                        
                        while(IsDigit(stream[0]) || IsPeriod(stream[0]) || IsMinusSign(stream[0]))
                        {
                            v3 normal{};
                            for(s32 i{}; i < 3; ++i)
                            {
                                char scalarString[10]{};
                                s32 stringI{};
                                while(IsDigit(stream[0]) || IsPeriod(stream[0]) || IsMinusSign(stream[0]))
                                {
                                    scalarString[stringI] = *stream++;
                                    ++stringI;
                                };
                                
                                normal.elem[i] = (f32)strtod(scalarString, NULL);
                                AdvanceStream(&stream);
                            };
                            
                            vertexNormals.Push(normal);
                            
                            while(stream[0] != 'v' && stream[0] != 'u')
                                AdvanceStream(&stream);
                        };
                        
                        BGZ_ASSERT(stream[0] != '\0', "Reached end of stream!");
                        
                        if(NOT IsDigit(Peek(stream, 3)))
                            notAllNormalsParsed = false;
                    };
                }
                else if(Peek(stream, 1) == ' ' && (IsDigit(Peek(stream, 2)) || IsMinusSign(Peek(stream, 2))))
                {
                    bool notAllVertsParsed{true};
                    while(notAllVertsParsed)
                    {
                        if(stream[0] == 'v')
                        {
                            //Remove initial prefix and whitespace
                            AdvanceStream(&stream);
                            AdvanceStream(&stream);
                            
                            while(IsDigit(stream[0]) || IsPeriod(stream[0]) || IsMinusSign(stream[0]))
                            {
                                v3 vector{};
                                for(s32 i{}; i < 3; ++i)
                                {
                                    char scalarString[10]{};
                                    s32 stringI{};
                                    while(IsDigit(stream[0]) || IsPeriod(stream[0]) || IsMinusSign(stream[0]))
                                    {
                                        scalarString[stringI] = *stream++;
                                        ++stringI;
                                    };
                                    
                                    vector.elem[i] = (f32)strtod(scalarString, NULL);
                                    AdvanceStream(&stream);
                                };
                                
                                vertexPositions.Push(vector);
                                
                                while(stream[0] != 'v')
                                    AdvanceStream(&stream);
                            };
                        }
                        
                        BGZ_ASSERT(stream[0] != '\0', "Reached end of stream!");
                        
                        if(Peek(stream, 1) == 't')
                            notAllVertsParsed = false;
                    }
                }
                else
                {
                    AdvanceStream(&stream);
                };
            }break;
            
            case 'f':
            {
                if(Peek(stream, 1) == ' ')
                {
                    //Remove initial prefix and whitespace
                    AdvanceStream(&stream);
                    AdvanceStream(&stream);
                    
                    s32 counter{};
                    bool notAllFacesParsed{true};
                    while(notAllFacesParsed)
                    {
                        char numString[10]{};
                        for(s32 i{}; IsDigit(stream[0]); ++i)
                        {
                            numString[i] = stream[0];
                            AdvanceStream(&stream);
                        };
                        
                        if(counter == 0)
                            vertexPositionIndicies.Push((s32)strtod(numString, NULL));
                        else if (counter == 1)
                            vertexTexCoordIndicies.Push((s32)strtod(numString, NULL));
                        else if (counter == 2)
                            vertexnormalIndicies.Push((s32)strtod(numString, NULL));
                        
                        if(stream[0] == '/')
                        {
                            ++counter;
                            AdvanceStream(&stream);
                        }
                        else if(stream[0] == ' ')
                        {
                            counter = 0;
                            AdvanceStream(&stream);
                        }
                        else if(IsWhiteSpace(Peek(stream, 1)))
                        {
                            counter = 0;
                            AdvanceStream(&stream);
                            AdvanceStream(&stream);
                            if(stream[0] == '\0')
                                break;
                            AdvanceStream(&stream);
                            if(stream[0] == '\0')
                                break;
                            AdvanceStream(&stream);
                        }
                        else if(Peek(stream, 1) == '\0' || stream[0] == '\0')
                        {
                            notAllFacesParsed = false;
                        };
                    };
                }
                else
                {
                    AdvanceStream(&stream);
                };
            }break;
            
            default:
            {
                AdvanceStream(&stream);
            }break;
        };
    };
    
    InitArr($(resultVertices), memPart, vertexPositionIndicies.length);
    for(s64 i{}; i < resultVertices.capacity; ++i)
    {
        resultVertices.Push();
        resultVertices[i].position = vertexPositions[vertexPositionIndicies[i] - 1];
        resultVertices[i].texCoord = vertexTexCoords[vertexTexCoordIndicies[i] - 1];
        resultVertices[i].normal = vertexNormals[vertexnormalIndicies[i] - 1];
    }
    
    return resultVertices;
};

void testOBJFileLoader(Memory_Partition* memPart)
{
    RunTimeArr<Vertex> temp{memPart, 36};//Only works for cube right now
    
    RunTimeArr<Vertex> temp2 = LoadOBJ("data/cube.obj", memPart);
    
    CopyArray(temp2, $(temp));
};


void test_MPCLibrary()
{
    {
        mpc_parser_t* parser = mpc_char('v');
        
        char* fileContents = globalPlatformServices->ReadEntireFile($(s32()), "data/cube.obj");
        
        mpc_result_t resultOfParse{};
        if(mpc_parse("fileContents", fileContents, parser, &resultOfParse))
        {
            BGZ_CONSOLE("%s", (char*)resultOfParse.output);
        }
        else if(*fileContents == 0)
        {
        }
        else
        {
            fileContents++;
        }
    }
    
    {
        const char *input =
            "# ? abcHVwufvyuevuy3y436782\n"
            "\n"
            "\n"
            "rehre\n"
            "rew\n"
            "-ql.;qa\n"
            "eg";
        
        mpc_parser_t* Line = mpc_many(
                                      mpcf_strfold,
                                      mpc_apply(mpc_re("[^\\n]*(\\n|$)"), read_line));
        
        mpc_result_t r;
        
        mpc_parse("input", input, Line, &r);
        printf("\nParsed String: %s", (char*)r.output);
        free(r.output);
        
        mpc_delete(Line);
    }
    
    {
        {
            const char *input = "  hello 4352 ,  \n foo.bar   \n\n  test:ing   ";
            
            mpc_parser_t* Tokens = mpc_many(
                                            mpcf_freefold,
                                            mpc_apply(mpc_strip(mpc_re("\\s[a-zA-Z_]+|[0-9]+|,|\\.|:)")), print_token));
            
            mpc_result_t r;
            mpc_parse("input", input, Tokens, &r);
            
            mpc_delete(Tokens);
        };
    };
    
    {
        mpc_parser_t *alpha = mpc_or(2, mpc_range('a', 'z'), mpc_range('A', 'Z'));
        mpc_parser_t *digit = mpc_range('0', '9');
        mpc_parser_t *underscore = mpc_char('_');
        
        mpc_parser_t *ident = mpc_and(2, mpcf_strfold,
                                      mpc_or(2, alpha, underscore),
                                      mpc_many(mpcf_strfold, mpc_or(3, alpha, digit, underscore)),
                                      free);
        
        /* Do Some Parsing... */
        const char *input = "  hello 4352 ,  \n foo.bar   \n\n  test:ing   ";
        mpc_result_t r;
        
        while(1)
        {
            mpc_parse("input", input, ident, &r);
            
            printf("\nParsed String: %s", (char*)r.output);
            
            input++;
        }
        
        mpc_delete(ident);
    };
};

#endif //OBJ_FILE_PARSER_IMPL