#ifndef OBJ_FILE_PARSER_H
#define OBJ_FILE_PARSER_H

#include <stdlib.h>
#include <mpc/mpc.h>

struct ObjFileData
{
    RunTimeArr<v3> verts{};
    RunTimeArr<s16> indicies{};
};

#endif //OBJ_FILE_PARSER_H

void test_MPCLibrary();

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
    VECTOR = 'v'
};

struct Token
{
    Token_Type type;
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

Token GetToken(Tokenizer&& tokenizer)
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
                AdvanceTokenizer($(tokenizer));
                if(IsWhiteSpace(tokenizer.at[0]))
                {
                    AdvanceTokenizer($(tokenizer));
                    if(IsDigit(tokenizer.at[0]) || IsPeriod(tokenizer.at[0]) || IsMinusSign(tokenizer.at[0]))
                    {
                        token.type = VECTOR;
                        contentsStillNeedParsed = false;
                    };
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

void ParseAndStoreContents(ObjFileData&& data, char* fileContents)
{
    Tokenizer tokenizer{};
    tokenizer.at = fileContents;
    
    bool contentsStillNeedParsed{true};
    while(contentsStillNeedParsed)
    {
        Token token = GetToken($(tokenizer));
        
        switch(token.type)
        {
            case VECTOR:
            {
                Array<f32, 3> scalars{};
                for(s32 i{}; i < 3; ++i)
                {
                    if(IsWhiteSpace(tokenizer.at[0]))
                        AdvanceTokenizer($(tokenizer));
                    
                    char scalarString[10]{};
                    s32 stringI{};
                    while(IsDigit(tokenizer.at[0]) || IsPeriod(tokenizer.at[0]) || IsMinusSign(tokenizer.at[0]))
                    {
                        scalarString[stringI] = tokenizer.at[0];
                        AdvanceTokenizer($(tokenizer));
                        ++stringI;
                    };
                    
                    scalars[i] = (f32)strtod(scalarString, NULL);
                };
                
                data.verts.Push();
                data.verts[data.verts.length - 1].x = scalars[0];
                data.verts[data.verts.length - 1].y = scalars[1];
                data.verts[data.verts.length - 1].z = scalars[2];
            }break;
            
            case END_OF_FILE:
            {
                contentsStillNeedParsed = false;
            }break;
        };
    };
};

ObjFileData LoadObjFileData(Memory_Partition* memPart, const char* filePath)
{
    s32 length{};
    char* fileContents = globalPlatformServices->ReadEntireFile($(length), filePath);
    BGZ_ASSERT(*(fileContents + length) == '\0', "No null termination at end of file!");
    
    ObjFileData data{};
    InitArr($(data.verts), memPart, 20/*capacity*/);
    InitArr($(data.indicies), memPart, 20/*capacity*/);
    
    ParseAndStoreContents($(data), fileContents);
    
    return data;
};

void ConstructGeometry(RunTimeArr<v3>&& verts, RunTimeArr<s16>&& indicies, ObjFileData objFileData)
{
    
};


void test_OBJFileParsing(Memory_Partition* memPart)
{
    ObjFileData data = LoadObjFileData(memPart, "data/cube.obj");
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