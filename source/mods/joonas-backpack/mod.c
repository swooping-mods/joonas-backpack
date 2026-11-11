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
    SURoot root,orange,capsule_mat,arms[4],brushes[2],storage;SMResources model;
    SPVec3 previous[2],display[2],normal[2];int contact[2],posed[2],equipped,registered;float phase,timer,retry,visual_retry;double kilograms;unsigned shots;
} bp;
static int feature_can_buy(PTR world){int ok=1;PTR player=call(tt.player,0,0,&ok);void *args[]={player};PTR owned=call(bp.levels,world,args,&ok);int total=integer(bp.total,world,&ok),levels=owned?*(int *)unbox(owned):0;if(!ok||levels<12||levels*2<total||total<1){tt.status="Late game: own at least 12 and half of all upgrade levels";return 0;}return 1;}
static PTR bp_part(PTR parent,SPVec3 p,SPVec3 scale,PTR material){return tt_primitive(parent,3,p,scale,material);}
static void bp_hide(void){
    su_destroy(su_get(bp.root));su_drop(&bp.root);
    for(unsigned i=0;i<4;i++)su_drop(&bp.arms[i]);
    for(unsigned i=0;i<2;i++){su_drop(&bp.brushes[i]);bp.posed[i]=bp.contact[i]=0;}
}
static int bp_visual(PTR walker,int show){
    if(!show){bp_hide();return 0;}if(bp.root)return 1;
    if(bp.visual_retry>0)return 0;bp.visual_retry=2;
    if(!bp.orange)bp.orange=su_hold(tt_material((SUColor){.88f,.45f,.10f,1}));
    if(!bp.capsule_mat)bp.capsule_mat=su_hold(tt_material((SUColor){.35f,.67f,.79f,1}));
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
static void bp_register(PTR world){
    if(bp.registered||!tt.owned||!tt_host())return;int ok=1;char id[128],player_id[128];PTR player=call(tt.player,0,0,&ok);if(!ok||!su_string(player,player_id,sizeof(player_id)))return;unsigned n=0;while(player_id[n])n++;char *p=cat(id,"moonlab.joonas-backpack.");num(p,tt_hash((unsigned char *)player_id,n));bp.storage=su_hold(su.string(id));double capacity=20;void *args[]={su_get(bp.storage),&capacity};call(bp.register_storage,world,args,&ok);if(ok){bp.registered=1;tt_event("storage-registered");}else su_drop(&bp.storage);
}
static double bp_mass(PTR world,int *ok){void *args[]={su_get(bp.storage)};PTR kg=call(bp.storage_mass,world,args,ok);if(!kg){*ok=0;return 0;}return *(double *)unbox(kg);}
static void bp_launch(PTR walker,PTR world){
    if(bp.retry>0||bp.kilograms<19.999)return;bp.retry=2;int ok=1,catcher=-1;SPVec3 up=su_vector(su.tr_up,tt_transform(walker)),from=su_add(su_vector(su.tr_get_position,tt_transform(walker)),su_mul(up,2.1f)),target={0},target_up={0};void *find[]={&from,&catcher,&target,&target_up};PTR result=call(bp.find_catcher,world,find,&ok);if(!ok||!result||!*(unsigned char *)unbox(result)){tt.status="Capsule full: waiting for a compatible catcher";return;}
    void *time_args[]={&from,&target};PTR duration=call(bp.flight,0,time_args,&ok);if(!ok||!duration)return;float arrival=su_number(bp.time,0)+*(float *)unbox(duration);void *reserve[]={&catcher,&arrival};PTR booked=call(bp.reserve,world,reserve,&ok);if(!ok||!booked)return;arrival=*(float *)unbox(booked);
    /* This original capsule uses the game's own J.O.O.N.A.S. flight component.
     * It owns routing, arrival, catcher animation and the actual dust deposit. */
    PTR go=su_gameobject("Backpack launched dust capsule",0);SURoot hold=su_hold(go);PTR capsule=tt_component(go,"MoonCleaners","MoonLaunchCapsule");PTR tr=su_transform(go);bp_part(tr,su_v(0,0,0),su_v(.35f,.7f,.35f),su_get(bp.capsule_mat));bp_part(tr,su_v(0,-.38f,0),su_v(.23f,.12f,.23f),su_get(bp.orange));
    PTR owner=call(tt.player,0,0,&ok);double dust=bp.kilograms<20?bp.kilograms:20;void *args[]={world,&catcher,&from,&up,&arrival,owner,&dust};if(capsule)call(bp.launch,capsule,args,&ok);else ok=0;
    if(ok){void *withdraw[]={su_get(bp.storage),&dust};PTR withdrawn=call(bp.take,world,withdraw,&ok);double taken=withdrawn?*(double *)unbox(withdrawn):0;if(!ok||taken!=dust)ok=0;}
    if(!ok){su_destroy(go);int release_ok=1;void *release[]={&catcher,&arrival};call(bp.forget,world,release,&release_ok);tt.status="Capsule launch refused";tt_event("launch-refused");}else{call(bp.request_save,world,0,&ok);bp.shots++;tt.status="Full capsule launched to catcher";tt_event("capsule-launched");}su_drop(&hold);
}
static void feature_drop(void){bp_hide();sm_release(&bp.model);su_destroy(su_get(bp.orange));su_destroy(su_get(bp.capsule_mat));su_drop(&bp.orange);su_drop(&bp.capsule_mat);su_drop(&bp.storage);bp.registered=bp.equipped=0;bp.timer=bp.retry=bp.phase=bp.visual_retry=0;bp.contact[0]=bp.contact[1]=0;bp.kilograms=0;bp.shots=0;}
static void feature_tick(PTR walker,PTR world,int allow){
    if(cfg.enabled)bp_register(world);bp.equipped=allow&&tt.owned;int active=allow&&bp.equipped&&tt.owned&&bp.registered&&tt_host();bp.visual_retry-=tt.dt;if(!bp_visual(walker,active))active=0;bp.timer+=tt.dt;bp.retry-=tt.dt;if(!active){bp.contact[0]=bp.contact[1]=0;tt_shop_text("Select backpack in Equipment\nOwn 20 kg capsule; full loads launch to normal catchers");return;}
    int ok=1;bp.kilograms=bp_mass(world,&ok);if(!ok)return;bp_launch(walker,world);SPVec3 origin=su_vector(su.tr_get_position,tt_transform(walker)),up=su_vector(su.tr_up,tt_transform(walker)),forward=su_vector(su.tr_forward,tt_transform(walker)),right=su_normal(su_cross(up,forward));
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
            call(bp.clean,world,args,&ok);bp.previous[i]=shown;bp.contact[i]=1;
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
