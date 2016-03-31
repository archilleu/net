//---------------------------------------------------------------------------
#include "json_reader.h"
#include "value.h"
#include "../function.h"
//---------------------------------------------------------------------------
namespace base
{

namespace json
{
//---------------------------------------------------------------------------
JsonReader::JsonReader()
{
}
//---------------------------------------------------------------------------
JsonReader::~JsonReader()
{
}
//---------------------------------------------------------------------------
bool JsonReader::Parse(const std::string& str, Value* root)
{
    return Parse(str.c_str(), root);
}
//---------------------------------------------------------------------------
bool JsonReader::Parse(const char* str, Value* root)
{
    token_reader_.set_dat(MemoryBlock(str, strlen(str)));
    return _Parse(root);
}
//---------------------------------------------------------------------------
bool JsonReader::Parse(MemoryBlock&& dat, Value* root)
{
    token_reader_.set_dat(std::move(dat));
    return _Parse(root);
}
//---------------------------------------------------------------------------
bool JsonReader::ParseFile(const std::string& path, Value* root)
{
    return ParseFile(path.c_str(), root);
}
//---------------------------------------------------------------------------
bool JsonReader::ParseFile(const char* path, Value* root)
{
    MemoryBlock dat;
    if(false == base::LoadFile(path, &dat))
        return false;

    token_reader_.set_dat(std::move(dat));
    return _Parse(root);
}
//---------------------------------------------------------------------------
bool JsonReader::_Parse(Value* )
{
    //解析JSON栈
    std::stack<Value> parse_stack;

    int status  = kSTATUS_OBJECT_BEGIN | kSTATUS_SINGLE_VALUE | kSTATUS_ARRAY_BEGIN;
    int token   = token_reader_.ReadNextToken();
    for(;;)
    {
        switch(token)
        {
            case TokenReader::TokenType::DOCUMENT_END:

                break;

            case TokenReader::TokenType::OBJECT_BEGIN:
                if(!HasStatus(kSTATUS_OBJECT_BEGIN))
                    return false;

                //{ -> "key" or { or }
                parse_stack.push(Value(Value::TYPE_OBJECT));
                status = kSTATUS_OBJECT_KEY | kSTATUS_OBJECT_BEGIN | kSTATUS_OBJECT_END;
                break;

            case TokenReader::TokenType::OBJECT_END:
                break;

            case TokenReader::TokenType::ARRAY_BEGIN:
                break;

            case TokenReader::TokenType::ARRAY_END:
                break;

            case TokenReader::TokenType::SEP_COLON: //:
                if(!HasStatus(kSTATUS_SEP_COLON))
                    return false;

                //: -> { or [ or "value"
                status = kSTATUS_OBJECT_BEGIN | kSTATUS_ARRAY_BEGIN | kSTATUS_OBJECT_VALUE;
                break;

            case TokenReader::TokenType::SEP_COMMA: //,
                if(!HasStatus(kSTATUS_SEP_COMMA))
                    return false;

                //, -> "key" or { or arr_val
                status = kSTATUS_OBJECT_KEY | kSTATUS_OBJECT_BEGIN | kSTATUS_ARRAY_VALUE;
                break;

            case TokenReader::TokenType::STRING:
                if(HasStatus(kSTATUS_OBJECT_KEY))
                {
                    std::string key;
                    bool err_code = token_reader_.ReadString(key);
                    if(false == err_code)
                        return false;
                    
                    Value value(Value::TYPE_KEY);
                    value.set_key(key);
                    parse_stack.push(std::move(value));

                    //期待下一个token为:
                    status = kSTATUS_SEP_COLON;
                }

                if(HasStatus(kSTATUS_OBJECT_VALUE))
                {
                    Value& top = parse_stack.top();
                    if(Value::TYPE_KEY != top.type())
                        return false;

                    std::string str;
                    bool err_code = token_reader_.ReadString(str);
                    if(false == err_code)
                        return false;

                    parse_stack.pop();
                    Value& object = parse_stack.top();
                    Value value(Value::TYPE_STRING);
                    value.set_str(str);
                    object.PairAdd(top.get_key(), value);

                    //"value" -> , or }
                    status = kSTATUS_SEP_COMMA | kSTATUS_OBJECT_END;
                }

                if(HasStatus(kSTATUS_ARRAY_VALUE))
                {
                }
                break;

            case TokenReader::TokenType::BOOLEAN:
                break;

            case TokenReader::TokenType::NUMBER:
                break;

            case TokenReader::TokenType::NUL:
                break;

            case TokenReader::TokenType::INVALID:
                break;
        }
    }
    return true;
}
//---------------------------------------------------------------------------
}//namespace json

}//namespace base
