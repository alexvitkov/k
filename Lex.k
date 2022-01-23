
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