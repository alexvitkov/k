
fn IsSpace(ch) {
   //      space        newline      tab
   return (ch == 32) | (ch == 10) | (ch == 9);
}

fn IsLetter(ch) {
   if (ch >= 97) & (ch <= 122) { return 1; } // a-z
   if (ch >= 65) & (ch <= 90)  { return 1; } // A-Z
   if ch == 95                 { return 1; } // underscore
   return 0;
}

fn IsDigit(ch) {
   return (ch >= 48) & (ch <= 57);
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
        set num = num + (get8(str + digit)) - 48;
 	set digit = digit + 1;
    }

    return num;
}