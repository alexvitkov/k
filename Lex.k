
fn IsSpace(ch) {
   //      space        newline      tab
   return (ch == 32) | (ch == 10) | (ch == 9);
}

fn IsLetter(ch) {
   if (ch >= 97) & (ch <= 122) { return 1; } // a-z
   if (ch >= 65) & (ch <= 90)  { return 1; } // A-Z
   if ch == 137                { return 1; } // underscore
   return 0;
}

fn IsDigit(ch) {
   return (ch >= 48) & (ch <= 57);
}