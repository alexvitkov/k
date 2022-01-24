externfn strcmp;

fn IsSpace(ch) {
   return (ch == ' ') | (ch == '\n') | (ch == '\t');
}

fn IsLetter(ch) {
   if (ch >= 'a') & (ch <= 'z') { return 1; }
   if (ch >= 'A') & (ch <= 'Z') { return 1; }
   if ch == '_'                 { return 1; } 
   return 0;
}

fn IsDigit(ch) {
   return (ch >= '0') & (ch <= '9');
}

fn StrLen(str) {
   var _str;
   set _str = str;

   while 1 {
       if (get8(_str)) == 0 {
           return _str - str;
       }
       set _str = _str + 1;
   }
}

fn StrToNum(str) {
    var num;
    var digit;
    var digits_count;

    set num = 0;
    set digit = 0;
    set digits_count = StrLen(str);

    while digit < digits_count {
        set num = num * 10;
        set num = num + (get8(str + digit)) - '0';
 	set digit = digit + 1;
    }

    return num;
}

fn InferTokenType(str) {
  if (strcmp(str, "if")) == 0 { return TOK_IF; }
  if (strcmp(str, "else")) == 0 { return TOK_ELSE; }
  if (strcmp(str, "while")) == 0 { return TOK_WHILE; }
  if (strcmp(str, "fn")) == 0 { return TOK_FN; }
  if (strcmp(str, "return")) == 0 { return TOK_RETURN; }
  if (strcmp(str, "set")) == 0 { return TOK_SET; }
  if (strcmp(str, "var")) == 0 { return TOK_VAR; }
  if (strcmp(str, "externfn")) == 0 { return TOK_EXTERNFN; }
  if (strcmp(str, "const")) == 0 { return TOK_CONST; }
  return TOK_ID;
}

fn GetTwoCharOperator(c1, c2) {
  if (c1 == '=') & (c2 == '=') { return TOK_DOUBLE_EQUAL; }
  if (c1 == '!') & (c2 == '=') { return TOK_NOT_EQUAL; }
  if (c1 == '&') & (c2 == '&') { return TOK_DOUBLE_AND; }
  if (c1 == '|') & (c2 == '|') { return TOK_DOUBLE_OR; }
  if (c1 == '>') & (c2 == '=') { return TOK_GREATER_THAN_EQUAL; }
  if (c1 == '<') & (c2 == '=') { return TOK_LESS_THAN_EQUAL; }
  if (c1 == '-') & (c2 == '>') { return TOK_ARROW; }
  return TOK_NONE;
}

fn GetSingleCharOperator(c) {
  if ((c == '&') | (c == '+') | (c == '-') | (c == '*')
    | (c == '/') | (c == '%') | (c == '|') | (c == '=')
    | (c == ';') | (c == '(') | (c == ')') | (c == '{')
    | (c == '}') | (c == ',') | (c == '<') | (c == '>'))
  {
    return c;
  }
  return TOK_NONE;
}