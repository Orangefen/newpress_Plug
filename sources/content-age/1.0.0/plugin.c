/* Apache-2.0. Build: zig cc -target wasm32-freestanding -O2 -nostdlib
 * -Wl,--no-entry -Wl,--export-memory -Wl,-z,stack-size=65536 -Wl,--initial-memory=262144
 * -Wl,--max-memory=33554432 -o plugin.wasm plugin.c
 * ABI: JSON input; packed i64 output is (pointer << 32) | length.
 */
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT(name) __attribute__((export_name(name)))
__attribute__((import_module("newpress"),import_name("host_call")))
extern u32 host_call(u32,u32,u32,u32);
static char input[65536],reply[65536],output[8192];
typedef struct {const char *p;u32 n;} json;
static int space(char c){return c==' '||c=='\n'||c=='\r'||c=='\t';}
static u32 skip(json s,u32 i){while(i<s.n&&space(s.p[i]))i++;return i;}
static u32 string_end(json s,u32 i){if(i>=s.n||s.p[i]!='"')return s.n;i++;while(i<s.n){if(s.p[i]=='\\'){i+=2;continue;}if(s.p[i++]=='"')return i;}return s.n;}
static u32 value_end(json s,u32 i){
 if(i>=s.n)return s.n;if(s.p[i]=='"')return string_end(s,i);
 if(s.p[i]=='{'||s.p[i]=='['){u32 depth=0;for(;i<s.n;i++){char c=s.p[i];if(c=='"'){i=string_end(s,i)-1;continue;}if(c=='{'||c=='[')depth++;if(c=='}'||c==']'){if(--depth==0)return i+1;}}return s.n;}
 while(i<s.n&&s.p[i]!=','&&s.p[i]!='}'&&s.p[i]!=']'&&!space(s.p[i]))i++;return i;
}
static u32 len(const char*s){u32 n=0;while(s[n])n++;return n;}
static int eq(json s,const char*raw){u32 n=len(raw);if(n!=s.n)return 0;for(u32 i=0;i<n;i++)if(s.p[i]!=raw[i])return 0;return 1;}
static json field(json s,const char*key){
 u32 i=skip(s,0);if(i>=s.n||s.p[i++]!='{')return (json){0,0};
 while((i=skip(s,i))<s.n&&s.p[i]!='}'){
  u32 a=i,b=string_end(s,i);if(b<=a+1||b>=s.n)return (json){0,0};
  json k={s.p+a+1,b-a-2};i=skip(s,b);if(s.p[i++]!=':')return (json){0,0};i=skip(s,i);u32 end=value_end(s,i);
  if(eq(k,key))return (json){s.p+i,end-i};i=skip(s,end);if(i<s.n&&s.p[i]==',')i++;else break;
 };return (json){0,0};
}
static u64 number(json s){u64 n=0;for(u32 i=0;i<s.n;i++){if(s.p[i]<'0'||s.p[i]>'9')return 0;n=n*10+(s.p[i]-'0');}return n;}
static u32 cursor;
static void add(json s){if(cursor+s.n>=sizeof(output))return;for(u32 i=0;i<s.n;i++)output[cursor++]=s.p[i];}
static void literal(const char*s){add((json){s,len(s)});}
static u64 result(){return ((u64)(u32)output<<32)|cursor;}
EXPORT("np_api_version") u64 version(void){return 1;}
EXPORT("np_alloc") u32 alloc(u32 n){return n<=sizeof(input)?(u32)input:0;}
EXPORT("np_free") void release(u32 p,u32 n){(void)p;(void)n;} /* fresh instance owns fixed buffers */
EXPORT("np_handle") u64 handle(u32 p,u32 n){
 cursor=0;if(p!=(u32)input||n>sizeof(input)){literal("{}");return result();}
 json in={(char*)p,n};const char*q="{\"method\":\"settings.get\"}";
 u32 size=host_call((u32)q,len(q),(u32)reply,sizeof(reply));if(size>sizeof(reply)){literal("{}");return result();}
 json settings=field((json){reply,size},"data");u64 days=number(field(settings,"days"));json title=field(settings,"title"),message=field(settings,"message");
 if(!days||!title.n||!message.n){literal("{}");return result();}
 int preview=eq(field(in,"event"),"\"admin.action\"");
 if(preview&&!eq(field(field(in,"payload"),"action"),"\"preview\"")){literal("{\"data\":{\"error\":\"unsupported action\"}}");return result();}
 if(!preview){u64 now=number(field(in,"now")),updated=number(field(field(in,"post"),"updated_at"));if(!updated||now<updated||now-updated<days*86400){literal("{}");return result();}}
 literal("{\"fragment\":{\"title\":");add(title);literal(",\"text\":");add(message);literal(",\"tone\":\"warning\"}}");return result();
}
