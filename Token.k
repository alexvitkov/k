const NULL = 0;
const TOK_NONE = 0;

const TOK_ID = 128;
const TOK_NUMBER = 129;
const TOK_STRING = 130;

// operators
const TOK_DOUBLE_EQUAL = 200;
const TOK_NOT_EQUAL = 201;
const TOK_GREATER_THAN = 202;
const TOK_LESS_THAN = 203;
const TOK_GREATER_THAN_EQUAL = 204;
const TOK_LESS_THAN_EQUAL = 205;
const TOK_DOUBLE_AND = 206;
const TOK_DOUBLE_OR = 207;
const TOK_ARROW = 208;

// keywords
const TOK_CONST = 1000;
const TOK_IF = 1001;
const TOK_ELSE = 1002;
const TOK_WHILE = 1003;
const TOK_FN = 1004;
const TOK_RETURN = 1005;
const TOK_VAR = 1006;
const TOK_SET = 1007;
const TOK_EXTERN = 1008;
const TOK_STATIC = 1009;
const TOK_SET8 = 1010;
const TOK_BREAK = 1011;
const TOK_CONTINUE = 1012;

const TOK_INFER_KEYWORD_OR_IDENTIFIER = 2000;

// Token struct
const TokenType = 0;
const TokenString = 8;
const TokenNumber = 16;
const Sizeof_Token = 24;