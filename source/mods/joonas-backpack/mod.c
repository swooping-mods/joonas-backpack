#include "../../runtime/native/gameplay.h"
#define TOOL_ID "joonas-backpack"
#define TOOL_DESCRIPTION "Late-game wearable sweeper arms with a separate dust capsule and native catcher launches."
#define TOOL_NAME "J.O.O.N.A.S. Backpack"
#define TOOL_PRICE 5000
#define TOOL_KEY 106
#define TOOL_ACTION 98
#define TOOL_SPEED 38.f
#define TOOL_ENV "MOONLAB_JOONAS_BACKPACK"
#define TOOL_HOTKEY "Alt+J"
#define TOOL_ROW 0
#include "tool_runtime.h"
#include "settings.h"
#include "../../runtime/sdk/scene/static_model_pbr.h"
#include "assets/generated/model_data.h"
#include "motion.h"
int _fltused=0;
void *memset(void *to,int value,U64 count){unsigned char *p=to;while(count--)*p++=(unsigned char)value;return to;}
static void config_initialize(void){cfg_start();}static int configured_enabled(void){return cfg.enabled;}
static struct {
    PTR time,visible_dust,levels,total,register_storage,storage_mass,clean,find_catcher,reserve,forget,take,launch,flight,component_go,terrain,request_save;
    SURoot root,orange,capsule_mat,arms[4],brushes[2],storage,capsule;SMResources model;
    SPVec3 previous[2],display[2],normal[2];int contact[2],posed[2],equipped,registered,suspended,cleanup_failed;char owner[128];float phase,timer,retry,visual_retry;double kilograms;unsigned shots;
} bp;
#include "state.h"
#include "capsule.h"
static void bp_hide(void){
    su_destroy(su_get(bp.root));su_drop(&bp.root);
    for(unsigned i=0;i<4;i++)su_drop(&bp.arms[i]);
    for(unsigned i=0;i<2;i++){su_drop(&bp.brushes[i]);bp.posed[i]=bp.contact[i]=0;}
}
static int bp_visual(PTR walker,int show){
    if(!show){bp_hide();return 0;}if(bp.root)return 1;
    if(bp.visual_retry>0)return 0;bp.visual_retry=2;
    if(!sm_prepare_pbr(&bpm_model,&bp.model,bpm_pbr,7))return 0;
    PTR root=sm_attach(&bpm_model,&bp.model,tt_transform(walker),2,"Wearable RoboScrub J.O.O.N.A.S.");
    bp.root=su_hold(root);if(!bp.root){su_destroy(root);return 0;}
    PTR transform=su_transform(root),get_child=su_method("UnityEngine","Transform","GetChild","System.Int32");int ok=1;
    for(int i=1;i<7;i++){
        void *args[]={&i};PTR child=call(get_child,transform,args,&ok);PTR go=child?call(bp.component_go,child,0,&ok):0;
        SURoot hold=su_hold(go);if(!ok||!hold){bp_hide();return 0;}
        unsigned side=(unsigned)(i-1)/3,role=(unsigned)(i-1)%3;
        if(role==2)bp.brushes[side]=hold;else bp.arms[2*side+role]=hold;
    }
    /* Uploaded parts are rigid and already fitted in metres. Never scale a
     * link to reach its target: bp_solve preserves both measured lengths. */
    bp.visual_retry=0;tt_event("model-attached");return 1;
}
static void bp_segment(PTR go,SPVec3 a,SPVec3 b,SPVec3 facing){
    PTR tr=su_transform(go);su_set_vector(su.tr_position,tr,a);
    su_set_quat(su.tr_rotation,tr,su_rotation(su_normal(su_sub(b,a)),su_mul(facing,-1)));
}
static int bp_ground(PTR world,SPVec3 at,SPVec3 up,SPVec3 *point,SPVec3 *surface){unsigned char hit[64];if(!tt_ray(su_add(at,su_mul(up,1.8f)),su_mul(up,-1),3.5f,hit))return 0;int ok=1;PTR collider=call(su.hit_collider,hit,0,&ok);void *args[]={collider};PTR terrain=call(bp.terrain,world,args,&ok);if(!ok||!terrain||!*(unsigned char *)unbox(terrain))return 0;SPVec3 normal;su_copy(point,hit+su.hit_point,sizeof(*point));su_copy(&normal,hit+su.hit_normal,sizeof(normal));void *height_args[]={point};PTR height=call(bp.visible_dust,world,height_args,&ok);if(!ok||!height)return 0;float h=*(float *)unbox(height);if(!(h>=0&&h<20))return 0;*point=su_add(*point,su_mul(up,h));*surface=su_normal(normal);return su_dot(up,*surface)>.85f;}
static void feature_drop(void){
    bp_hide();sm_release(&bp.model);
    bp.registered=bp.equipped=0;su_drop(&bp.storage);bp.owner[0]=0;
    bp.timer=bp.retry=bp.phase=bp.visual_retry=0;bp.kilograms=0;bp.shots=0;
    (void)bp_capsule_release();
}
static void feature_tick(PTR walker,PTR world,int allow){
    if(allow)bp_register(world);bp.equipped=allow&&tt.owned;int active=allow&&bp.equipped&&bp.registered&&bp_authorized(world);bp.visual_retry-=tt.dt;if(!bp_visual(walker,active))active=0;bp.timer+=tt.dt;bp.retry-=tt.dt;if(!active){bp.contact[0]=bp.contact[1]=0;tt_shop_text(bp.suspended?(tt.status?tt.status:"Backpack suspended after cleanup failure; restart required"):"Select backpack in Equipment\nOwn 20 kg capsule; full loads launch to normal catchers");return;}
    int ok=1;bp.kilograms=bp_mass(world,&ok);if(!ok){bp_suspend("Storage mass unavailable or invalid");return;}bp_launch(walker,world);if(!bp_authorized(world))return;SPVec3 origin=su_vector(su.tr_get_position,tt_transform(walker)),up=su_vector(su.tr_up,tt_transform(walker)),forward=su_vector(su.tr_forward,tt_transform(walker)),right=su_normal(su_cross(up,forward));
    bp.phase+=tt.dt*.6f;if(bp.phase>=8)bp.phase-=8;
    int sweep=bp.timer>=.2f;if(sweep)bp.timer=0;
    for(unsigned i=0;i<2;i++){
        float side=i?1.f:-1.f;
        SPVec3 shoulder=su_add(origin,bp_frame(bpm_arm[i].shoulder,right,up,forward));
        /* Keep each brush in its own reachable side sector, clear of the
         * player's legs. The old full-radius sweep exceeded these arm lengths. */
        SPVec3 probe=su_add(origin,bp_probe(bp.phase,i,right,forward));
        SPVec3 point,surface=up;int contact=bp_ground(world,probe,up,&point,&surface);
        if(!contact)point=su_add(probe,su_mul(up,.15f));
        unsigned char hit[64];SPVec3 obstacle=su_add(point,su_mul(up,.3f)),reach=su_sub(obstacle,shoulder);float distance=su_sqrt(su_dot(reach,reach));
        if(tt_ray(shoulder,su_normal(reach),distance-.2f,hit)){contact=0;point=su_add(probe,su_mul(up,.35f));surface=up;}
        if(!bp.posed[i]){bp.display[i]=point;bp.normal[i]=surface;bp.posed[i]=1;}
        float alpha=tt.dt/(.09f+tt.dt);
        bp.display[i]=su_add(bp.display[i],su_mul(su_sub(point,bp.display[i]),alpha));
        bp.normal[i]=su_normal(su_add(bp.normal[i],su_mul(su_sub(surface,bp.normal[i]),alpha)));
        SPVec3 brush_up=bp.normal[i],brush_forward=su_normal(su_sub(forward,su_mul(brush_up,su_dot(forward,brush_up)))),brush_right=su_normal(su_cross(brush_up,brush_forward));
        SPVec3 wrist_offset=bp_frame(bpm_arm[i].wrist,brush_right,brush_up,brush_forward),target=su_add(bp.display[i],wrist_offset),elbow,wrist;
        int reached=bp_solve(shoulder,target,su_add(su_mul(right,side),su_mul(up,.25f)),bpm_arm[i].upper,bpm_arm[i].lower,&elbow,&wrist);
        bp_segment(su_get(bp.arms[2*i]),shoulder,elbow,forward);bp_segment(su_get(bp.arms[2*i+1]),elbow,wrist,forward);
        PTR brush=su_transform(su_get(bp.brushes[i]));SPVec3 shown=su_sub(wrist,wrist_offset);
        su_set_vector(su.tr_position,brush,shown);su_set_quat(su.tr_rotation,brush,su_rotation(brush_forward,brush_up));
        SPVec3 error=su_sub(shown,point);contact=contact&&reached&&su_dot(error,error)<.0004f;
        if(sweep&&contact&&bp.kilograms<19.999){
            SPVec3 from=bp.contact[i]?bp.previous[i]:shown;SPVec3 delta=su_sub(from,shown);if(su_dot(delta,delta)>.16f)from=shown;
            float width=bpm_arm[i].width,depth=bpm_arm[i].depth,dose=.07f,edge=.04f;PTR player=call(tt.player,0,0,&ok);
            void *args[]={&from,&shown,&brush_right,&brush_right,&width,&depth,&dose,&edge,&brush_forward,player,su_get(bp.storage)};
            if(!ok||!player){bp_suspend("Cleaning player unavailable");return;}
            if(!bp_clean_stroke(world,args))return;
            bp.previous[i]=shown;bp.contact[i]=1;
        }else if(!contact)bp.contact[i]=0;
    }
    char b[300],*p=cat(b,"Autonomous sweeper arms\nCapsule ");p=num(p,(U64)bp.kilograms);p=cat(p," / 20 kg  |  shots ");p=num(p,bp.shots);if(tt.status){p=cat(p,"\n");cat(p,tt.status);}tt_shop_text(b);
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(!tt_start(domain,game))return 0;bp.time=su_method("UnityEngine","Time","get_time","");bp.component_go=su_method("UnityEngine","Component","get_gameObject","");if(!bp.time||!bp.component_go)return 0;
#define BP(F,T,N,S,TOKEN) bp.F=su_method("MoonCleaners",T,N,S);if(!bp.F||method_token(bp.F)!=TOKEN)return 0
    BP(visible_dust,"MoonWorld","VisibleDustHeight","UnityEngine.Vector3",100664810);BP(levels,"MoonWorld","TreeLevelsOwned","System.String",100665709);BP(total,"MoonWorld","TreeLevelsTotal","",100665710);BP(register_storage,"MoonWorld","RegisterVehicleStorage","System.String;System.Double",100665755);BP(storage_mass,"MoonWorld","VehicleDustKilograms","System.String",100665757);
    BP(clean,"MoonWorld","CleanToolStroke","UnityEngine.Vector3;UnityEngine.Vector3;UnityEngine.Vector3;UnityEngine.Vector3;System.Single;System.Single;System.Single;System.Single;UnityEngine.Vector3;System.String;System.String",100664817);
    BP(find_catcher,"MoonWorld","FindCatcher","UnityEngine.Vector3;System.Int32&;UnityEngine.Vector3&;UnityEngine.Vector3&",100664770);BP(reserve,"MoonWorld","ReserveCatch","System.Int32;System.Single",100664784);BP(forget,"MoonWorld","ForgetCatch","System.Int32;System.Single",100664789);BP(take,"MoonWorld","TakeVehicleDust","System.String;System.Double",100664791);
    BP(launch,"MoonLaunchCapsule","Launch","MoonCleaners.MoonWorld;System.Int32;UnityEngine.Vector3;UnityEngine.Vector3;System.Single;System.String;System.Double",100667890);BP(flight,"MoonLaunchCapsule","FlightSeconds","UnityEngine.Vector3;UnityEngine.Vector3",100667885);BP(terrain,"MoonWorld","IsTerrainCollider","UnityEngine.Collider",100664730);BP(request_save,"MoonWorld","RequestSave","",100665566);
#undef BP
    return tt_install(game);
}
