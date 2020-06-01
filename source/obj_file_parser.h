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
    
    if(*string != '\0')
        string -= howFarAheadToPeek;
    
    return result;
};

Token GetToken(Tokenizer&& tokenizer, Memory_Partition* memPart)
{
    Token token{};
    token.length = 1;
    
    bool contentsStillNeedParsed{true};
    while(contentsStillNeedParsed)
    {
        switch(tokenizer.at[0])
        {
            case '\0':
            {
                token.type = END_OF_FILE;
                contentsStillNeedParsed = false;
            }break;
            
            case 'v':
            {
                if(IsWhiteSpace(Peek(tokenizer.at, 1)) && (IsDigit(Peek(tokenizer.at, 2)) || IsMinusSign(Peek(tokenizer.at, 2))))
                {
                    token.type = VECTOR;
                    contentsStillNeedParsed = false;
                    
                    //Remove v and initial whitespace
                    AdvanceTokenizer($(tokenizer));
                    AdvanceTokenizer($(tokenizer));
                    
                    char* scalarString = PushType(memPart, char, 100);
                    s32 stringI{}, sizeOfString{};
                    while(tokenizer.at[0] != 'v')
                    {
                        scalarString[stringI] = tokenizer.at[0];
                        AdvanceTokenizer($(tokenizer));
                        ++stringI;
                        ++sizeOfString;
                    };
                    
                    token.text = scalarString;
                    token.length = sizeOfString;
                }
                else
                {
                    AdvanceTokenizer($(tokenizer));
                };
            }break;
            
            case 'f':
            {
                if(IsWhiteSpace(Peek(tokenizer.at, 1)) && IsDigit(Peek(tokenizer.at, 2)))
                {
                    token.type = FACE;
                    contentsStillNeedParsed = false;
                    
                    //Remove f and initial whitespace
                    AdvanceTokenizer($(tokenizer));
                    AdvanceTokenizer($(tokenizer));
                    
                    char* indexString = PushType(memPart, char, 100);
                    
                    bool indiciesStillNeedLoaded{true};
                    s32 stringI{};
                    while(indiciesStillNeedLoaded)
                    {
                        while(IsDigit(tokenizer.at[0]))
                        {
                            indexString[stringI] = tokenizer.at[0];
                            AdvanceTokenizer($(tokenizer));
                            ++stringI;
                        };
                        
                        indexString[stringI] = ',';
                        ++stringI;
                        
                        AdvanceTokenizer($(tokenizer));
                        
                        while(NOT IsWhiteSpace(tokenizer.at[0]) && tokenizer.at[0] != '\0')
                            AdvanceTokenizer($(tokenizer));
                        
                        while(IsWhiteSpace(tokenizer.at[0]) || tokenizer.at[0] == 'f')
                            AdvanceTokenizer($(tokenizer));
                        
                        if(tokenizer.at[0] == '\0')
                            indiciesStillNeedLoaded = false;
                    };
                    
                    token.text = indexString;
                }
                else
                {
                    AdvanceTokenizer($(tokenizer));
                };
            }break;
            
            default:
            {
                AdvanceTokenizer($(tokenizer));
            }break;
        };
    }
    
    return token;
};

void ParseAndStoreContents(ObjFileData&& data, Memory_Partition* memPart, char* fileContents)
{
    Tokenizer tokenizer{};
    tokenizer.at = fileContents;
    
    Temporary_Memory parsingMemory = BeginTemporaryMemory($(*memPart));
    
    bool contentsStillNeedParsed{true};
    while(contentsStillNeedParsed)
    {
        Token token = GetToken($(tokenizer), memPart);
        
        switch(token.type)
        {
            case VECTOR:
            {
                BGZ_ASSERT(token.text != nullptr, "string should exist!");
                
                v3 vector{};
                for(s32 i{}; i < 3; ++i)
                {
                    char scalarString[10]{};
                    s32 stringI{};
                    while(IsDigit(*token.text) || IsPeriod(*token.text) || IsMinusSign(*token.text))
                    {
                        scalarString[stringI] = *token.text++;
                        ++stringI;
                    };
                    
                    vector.elem[i] = (f32)strtod(scalarString, NULL);
                    token.text++;//Move past whitespace
                };
                
                data.verts.Push(vector);
            }break;
            
            case FACE:
            {
                BGZ_ASSERT(token.text != nullptr, "string should exist!");
                
                bool stillIndiciesToStore{true};
                while(stillIndiciesToStore)
                {
                    char* placeholder{};
                    s32 num = strtol(&token.text[0], &placeholder, 10/*base*/);
                    data.indicies.Push(num - 1);//Index needs to start from a 0 base (0, 1, 2, 3) for opengl. Obj file has it starting at 1 for some reason.
                    
                    if(num > 9)
                    {
                        ++token.text;
                        ++token.text;
                    }
                    else
                    {
                        ++token.text;
                    };
                    
                    if(Peek(token.text, 1) != '\0')
                        ++token.text;
                    else
                        stillIndiciesToStore = false;
                }
            }break;
            
            case END_OF_FILE:
            {
                contentsStillNeedParsed = false;
            }break;
        };
    };
    
    EndTemporaryMemory(parsingMemory);
};

ObjFileData LoadObjFileData(Memory_Partition* memPart, const char* filePath)
{
    s32 length{};
    char* fileContents = globalPlatformServices->ReadEntireFile($(length), filePath);
    
    //TODO: Just over estimate vertex and index capacity and try and load in more complicated geometry
    ObjFileData data{};
    InitArr($(data.verts), memPart, 100/*capacity*/);
    InitArr($(data.indicies), memPart, 300/*capacity*/);
    
    ParseAndStoreContents($(data), memPart, fileContents);
    
    return data;
};

void ConstructGeometry(RunTimeArr<v3>&& verts, RunTimeArr<s16>&& indicies, ObjFileData objFileData)
{
    CopyArray(objFileData.verts, $(verts));
    CopyArray(objFileData.indicies, $(indicies));
};

void test_OBJFileParsing(Memory_Partition* memPart)
{
    RunTimeArr<v3> cubeVerts{};
    RunTimeArr<s16> cubeIndicies{};
    InitArr($(cubeVerts), memPart, 20);
    InitArr($(cubeIndicies), memPart, 50);
    
    ObjFileData data = LoadObjFileData(memPart, "data/cube.obj");
    ConstructGeometry($(cubeVerts), $(cubeIndicies), data);
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