#ifndef OBJ_FILE_PARSER_H
#define OBJ_FILE_PARSER_H

#include <mpc/mpc.h>

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

enum Token_Type
{
    END_OF_FILE = 0,
    TOKEN_V = 'v'
};

struct Token
{
    Token_Type type;
    char* text{};
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

bool IsDigitOrPeriodOrMinus(char character)
{
    bool result = ((character == '-'||
                    character == '.'||
                    character == '0'||
                    character == '1'||
                    character == '2'||
                    character == '3'||
                    character == '4'||
                    character == '5'||
                    character == '6'||
                    character == '7'||
                    character == '8'||
                    character == '9'));
    
    return result;
};

void AdvanceTokenizer(Tokenizer&& tokenizer)
{
    BGZ_ASSERT(*tokenizer.at != '\0', "Reached end of file!");
    ++tokenizer.at;
};

void Parse(char* fileContents)
{
    Tokenizer tokenizer{};
    tokenizer.at = fileContents;
    
    bool contentsStillNeedParsed{true};
    while(contentsStillNeedParsed)
    {
        switch(tokenizer.at[0])
        {
            case TOKEN_V:
            {
                AdvanceTokenizer($(tokenizer));
                while(IsWhiteSpace(tokenizer.at[0]))
                {
                    AdvanceTokenizer($(tokenizer));
                    while(IsDigitOrPeriodOrMinus(tokenizer.at[0]))
                    {
                        BGZ_CONSOLE("%c", tokenizer.at[0]);
                        AdvanceTokenizer($(tokenizer));
                    };
                };
            }break;
            
            case END_OF_FILE:
            {
                contentsStillNeedParsed = false;
            }break;
            
            default:
            {
                AdvanceTokenizer($(tokenizer));
            }break;
        };
    }
};

void test_OBJFileParsing()
{
    s32 length{};
    char* fileContents = globalPlatformServices->ReadEntireFile($(length), "data/cube.obj");
    BGZ_ASSERT(*(fileContents + length) == '\0', "No null termination at end of file!");
    
    Parse(fileContents);
};

#endif //OBJ_FILE_PARSER_IMPL