%{

#include "token_instance.h"

#define U3CHAR_TO_UCS2(input) ((input[0] & 0x0F) << 12 | (input[1] & 0x3F) << 6 | (input[2] & 0x3F))

%}

ASC     [\x00-\x7f]
HALFWIDTH_ALPHA [A-Za-z]
FULLWIDTH_ALPHA Ａ|Ｂ|Ｃ|Ｄ|Ｅ|Ｆ|Ｇ|Ｈ|Ｉ|Ｊ|Ｋ|Ｌ|Ｍ|Ｎ|Ｏ|Ｐ|Ｑ|Ｒ|Ｓ|Ｔ|Ｕ|Ｖ|Ｗ\Ｘ|Ｙ|Ｚ|ａ|ｂ|ｃ|ｄ|ｅ|ｆ|ｇ|ｈ|ｉ|ｊ|ｋ|ｌ|ｍ|ｎ|ｏ|ｐ|ｑ|ｒ|ｓ|ｔ|ｕ|ｖ|ｗ|ｘ|ｙ|ｚ
ALPHA {HALFWIDTH_ALPHA}|{FULLWIDTH_ALPHA}

U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]

CR \r
LF \n
SPACE " "|\t

DIGIT ([0-9]|０|１|２|３|４|５|６|７|８|９)
PERIOD \.|。
COMMA ,|，
EXCLAMATION !|！
QUESTION \?|？
PUNCTION ":"|"/"|"\\"|"."|"?"|"%"|"="|"&"|"#"|"!"|"~"|"-"|"@"
ALPHA_BLOCK {ALPHA}|{PUNCTION}
INTERPUNCT ·
QUOTE \"|\'|“|”

%%

({CR}|{LF})+ {
    return TokenInstance::kCrLf;
}

{DIGIT}+(\.{DIGIT}+)? {
    return TokenInstance::kNumber;
}

{ALPHA}+ {
    return TokenInstance::kEnglishWord;
}

({ALPHA_BLOCK}|{DIGIT})({ALPHA_BLOCK}|{DIGIT})+ {
    return TokenInstance::kSymbol;
}

{PUNCTION} {
    return TokenInstance::kPunctuation;
}

{PERIOD} {
    return TokenInstance::kPeriod;    
}

{COMMA} {
    return TokenInstance::kPunctuation;      
}

{EXCLAMATION} {
    return TokenInstance::kPeriod;        
}

{QUESTION} {
    return TokenInstance::kPeriod;      
}

{QUOTE} {
    return TokenInstance::kPunctuation;      
}

{INTERPUNCT} {
    return TokenInstance::kPunctuation; 
}

{U3}{U}{U} {
    int ord = U3CHAR_TO_UCS2(yytext);
    if (ord <= 0x9fa5 && ord >= 0x4e00) {
        return TokenInstance::kChineseChar;
    } else {
        return TokenInstance::kOther;
    }   
}

{U2}{U} {
    return TokenInstance::kOther; 
}

{U4}{U}{U}{U} {
    return TokenInstance::kOther; 
}

{SPACE}+ {
    return TokenInstance::kSpace;
}

{ASC} {
    return TokenInstance::kOther;     
}

<<EOF>> {
    return TokenInstance::kEnd;
}



%%


