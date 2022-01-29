extern strcmp;
extern malloc;
extern memcpy;
extern putchar;

fn IsSpace(ch) {
   return (ch == ' ') | (ch == '\n') | (ch == '\t');
}

fn IsLetter(ch) {
   if (ch >= 'a') & (ch <= 'z') { return TRUE; }
   if (ch >= 'A') & (ch <= 'Z') { return TRUE; }
   if ch == '_'                 { return TRUE; } 
   return FALSE;
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
  if (strcmp(str, "set8")) == 0 { return TOK_SET8; }
  if (strcmp(str, "var")) == 0 { return TOK_VAR; }
  if (strcmp(str, "extern")) == 0 { return TOK_EXTERN; }
  if (strcmp(str, "const")) == 0 { return TOK_CONST; }
  if (strcmp(str, "static")) == 0 { return TOK_STATIC; }
  if (strcmp(str, "break")) == 0 { return TOK_BREAK; }
  if (strcmp(str, "continue")) == 0 { return TOK_CONTINUE; }
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


fn MakeToken(file, offset, length, type) {
  var tok;
  set tok = malloc(Sizeof_Token);

  set tok->TokenString = (malloc(length + 1));
  memcpy((tok->TokenString), (file + offset), length);
  set8 ((tok->TokenString) + length) = 0;
  
  if (type == TOK_INFER_KEYWORD_OR_IDENTIFIER) {
    set tok->TokenType = InferTokenType(tok->TokenString);
  }
  else {
    if (type == TOK_NUMBER) {
      set tok->TokenNumber = StrToNum(tok->TokenString);
      set tok->TokenType   = TOK_NUMBER;
    } else {
      set tok->TokenType = type;
    }
  }
  
  return tok;
}

fn LexCharacterLiteral(file, offset_ptr) {
  var tok;
  var offset;
  var cur;

  set tok = NULL;
  set offset = get(offset_ptr);

  if ((get8(file + offset)) != '\'') { return NULL; }

  set offset = offset + 1;

  if (get8(file + offset)) == '\\' {
    set tok = MakeToken(file, get(offset_ptr), 3, TOK_NONE);
    set offset = offset + 1;

    set cur = get8(file + offset);

    if cur == 'n' { set tok->TokenNumber = '\n'; }
    if cur == 't' { set tok->TokenNumber = '\t'; }
    if cur == '\'' { set tok->TokenNumber = '\''; }
    if cur == '\\' { set tok->TokenNumber = '\\'; }
  }
  else {
    set tok = MakeToken(file, get(offset_ptr), 2, TOK_NONE);
    set tok->TokenNumber = get8(file + offset);
  }

  set offset = offset + 2;

  set tok->TokenType = TOK_NUMBER;
  set offset_ptr->NULL = offset;

  return tok;
}

fn LexString(file, offset_ptr) {
  var tok;
  var offset;

  set tok = NULL;
  set offset = get(offset_ptr);

  if (get8(file + offset)) != '"' { return NULL; }
  set offset = offset + 1;

  while (get8(file + offset)) != '"' {
    set offset = offset + 1;
  }

  set tok = MakeToken(file, (get(offset_ptr)) + 1, offset - (get(offset_ptr)) - 1, TOK_STRING);

  set offset = offset + 1;
  set offset_ptr->NULL = offset;
  return tok;
}


fn LexFile(file) {
  var list;
  var i;
  var tok;
  var tt;
  var is_in_word;
  var is_in_number;
  var word_start;
 
  set list         = 0;
  set i            = 0;
  set is_in_word   = FALSE;
  set is_in_number = FALSE;
  set word_start   = -1;
 
  while (1) {
    // Lex comments
    if (get8(file+i) == '/') & (get8(file+i) == '/') {
      while get8(file+i) != '\n' {
        set i = i + 1;
      }
      continue;
    }
 
    // Character literal
    set tok = LexCharacterLiteral(file, addr(i));
    if tok != NULL {
      set list = Append(addr(list), tok);
      continue;
    }
 
    // String
    set tok = LexString(file, addr(i));
    if tok {
      set list = Append(addr(list), tok);
      continue;
    }
 
    // Lex identifier
    if (is_in_word) {
      if (IsLetter(get8(file+i)) == FALSE) & (IsDigit(get8(file+i)) == FALSE) {
        set is_in_word = FALSE;
        set list = Append(addr(list), MakeToken(file, word_start, i - word_start, TOK_INFER_KEYWORD_OR_IDENTIFIER));
      }
      else {
        set i = i + 1;
        continue;
      }
    }
    else {
      if (IsLetter(get8(file+i))) {
        set is_in_word = TRUE;
        set word_start = i;
        set i = i + 1;
        continue;
      }
    }
 
    // Lex number
    if IsDigit(get8(file+i)) {
      if is_in_number == FALSE {
        set is_in_number = TRUE;
        set word_start   = i;
      }
      set i = i + 1;
      continue;
    } else {
      if (is_in_number) {
        set is_in_number = FALSE;
        set list         = Append(addr(list), MakeToken(file, word_start, i - word_start, TOK_NUMBER));
      }
    }
 
    // Lex 2-char op
    set tt = GetTwoCharOperator(get8(file+i), get8(file + i + 1));
    if tt != TOK_NONE {
      set list = Append(addr(list), MakeToken(file, i, 2, tt));
      set i = i + 2;
      continue;
    }


    // Lex 1-char op
    set tt = GetSingleCharOperator(get8(file+i));
    if tt != TOK_NONE {
      set list = Append(addr(list), MakeToken(file, i, 1, tt));
      set i = i + 1;
      continue;
    }
 
    if (IsSpace(get8(file+i))) {
      set i = i + 1;
      continue;
    }
 
    if get8(file+i) == 0 {
      break;
    }
 
    fprintf(stderr, "Unexpected character '%c'\n", get8(file+i));
    return NULL;
  }
 
  return list;
}
