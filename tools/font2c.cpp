#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <bitset>
#include <cctype>
using namespace std;

int tocode(const string&s){
    int ret=0;
    for(const auto&x:s){
        ret*=16;
        if(isdigit(x))ret+=x-'0';
        else if(islower(x))ret+=x-'a'+10;
        else if(isupper(x))ret+=x-'A'+10;
        else return 0;
    }
    return ret;
}

int main(int argc,char**argv){
    if(argc!=3)return -1;
    const string src(argv[1]),dst(argv[2]);
    ifstream Fdata(src);
    map<int,vector<uint8_t>>dbuf;
    string buf;
    int p=0;
    int code=0;
    while(getline(Fdata,buf)){
        if(p==0){
            if(buf.starts_with("STARTCHAR")){
                p=1;
                stringstream sstr;
                sstr<<buf;
                string token,pp;
                sstr>>token>>pp;
                code=tocode(pp);
            }
        }else{
            if(p==1){
                if(buf.starts_with("BITMAP")){
                    p=2;
                }
                continue;
            }
            if(buf.starts_with("ENDCHAR")){
                p=0;
                continue;
            }
            uint8_t data=0;
            for(int i=0;i<buf.size();++i)
                if(buf[i]=='@')
                    data|=1<<(7-i);
            dbuf[code].push_back(data);
        }
    }
    ofstream ofs(dst);
    ofs<<"#ifndef SOS_KERNEL_ASCII_FONT\n";
    ofs<<"#define SOS_KERNEL_ASCII_FONT\n";
    ofs<<"unsigned char Font[128][16]={\n";
    for(int i=0;i<128;++i){
        if(dbuf.find(i)!=dbuf.end()){
            ofs<<"{\n";
            for(const auto&x:dbuf[i])
                ofs<<"  0b"<<bitset<8>(x).to_string()<<",\n";
            ofs<<"},\n";
        }else ofs<<"{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},\n";
    }
    ofs<<"};\n";
    ofs<<"#endif\n";
}