/* Per-mod configuration; file defaults can be overridden by the environment. */
#ifndef GADGET_SETTINGS_H
#define GADGET_SETTINGS_H
static struct {int enabled,key,action;float speed;} cfg={1,TOOL_KEY,TOOL_ACTION,TOOL_SPEED};
static inline int cfg_integer(const char *v){int n=0;while(*v>='0'&&*v<='9'){n=n*10+*v++-'0';if(n>10000)return 10000;}return n;}
static inline void cfg_pair(const char *key,const char *value){int n=cfg_integer(value);if(su_eq(key,"enabled"))cfg.enabled=n!=0;else if(su_eq(key,"equipment_key")&&n>=32&&n<=329)cfg.key=n;else if(su_eq(key,"action_key")&&n>=32&&n<=329)cfg.action=n;else if(su_eq(key,"speed")&&n>=8&&n<=65)cfg.speed=(float)n;}
static inline void cfg_start(void){
    char bytes[2048]={0},path[1024];DWORD path_size=GetEnvironmentVariableA(TOOL_ENV "_CONFIG",path,sizeof(path));if(!path_size||path_size>=sizeof(path))cat(path,"MoonLab\\mods\\" TOOL_ID "\\settings.ini");PTR file=CreateFileA(path,0x80000000,1,0,3,0x80,0);
    if(file!=(PTR)(U64)-1){DWORD n=0;if(tt.read(file,bytes,sizeof(bytes)-1,&n,0)&&n<sizeof(bytes))bytes[n]=0;tt.close(file);char *p=bytes;while(*p){char key[64]={0},value[64]={0};unsigned i=0;while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n')p++;while(*p&&*p!='='&&*p!='\n'&&i<63)key[i++]=*p++;if(*p=='='){p++;i=0;while(*p&&*p!='\n'&&i<63)value[i++]=*p++;cfg_pair(key,value);}while(*p&&*p!='\n')p++;}}
    char env[100];const char *names[]={TOOL_ENV "_ENABLED",TOOL_ENV "_KEY",TOOL_ENV "_ACTION_KEY",TOOL_ENV "_SPEED"},*keys[]={"enabled","equipment_key","action_key","speed"};
    for(unsigned i=0;i<4;i++){DWORD n=GetEnvironmentVariableA(names[i],env,sizeof(env));if(n&&n<sizeof(env))cfg_pair(keys[i],env);}tt_event("settings-loaded");
}
#endif
