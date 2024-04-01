#include <iostream>

using namespace std;

struct ParsedResult {
    int num{0};
    int parsedLen{0};
    char c;
};
struct Window {
    int lpos;
    int len;
    char c;
};
ParsedResult Parse(char *str) {
    int parsedLen = 0;
    int num = atoi(str);
    while (*str != '\0' && *str >= '0' && *str <= '9') {
        ++parsedLen;
        ++str;
    }
    char c = *str;
    parsedLen +=1;
    return {num, parsedLen, c};
}

int GetComNum(const Window &l1, const Window &l2){
    if (l1.c != l2.c) {
        return 0;
    }
    int l1End = l1.lpos + l1.len;
    int l2End = l2.lpos + l2.len;
    if (l1End <= l2.lpos || l2End <= l1.lpos) {
        return 0;
    }
    int left = std::max(l1.lpos, l2.lpos);
    int right = std::min(l1End, l2End);
    return right - left;
}

float GetCommonRate(char *str1, char *str2) {
    Window l , r;
    ParsedResult lr = Parse(str1);
    ParsedResult rr = Parse(str2);
    l = {0, lr.num, lr.c};
    r = {0, rr.num, rr.c};
    int llen = lr.num;
    int rlen = rr.num;
    int commRet = 0;
    while (*str1 != '\0' && *str2 != '\0') {
        commRet += GetComNum(l, r);
        if (l.lpos + l.len < r.lpos + r.len) {
            str1 += lr.parsedLen;
            lr = Parse(str1);
            l = {l.lpos + l.len, lr.num, lr.c};
            llen += lr.num;
        } else {
            str2 += rr.parsedLen;
            rr = Parse(str2);
            r = {r.lpos + r.len, rr.num, rr.c};
            rlen += rr.num;
        }
    }
    return float(commRet) / std::max(llen, rlen);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "error" << endl;
        return 1;
    }
    cout << GetCommonRate(argv[1], argv[2]) << endl;
    return 0;
}