/* Native capsule preflight and bounded failure handling. Native Launch and
 * TakeVehicleDust are independent mutations, not an atomic transaction. */
#ifndef MOON_BACKPACK_CAPSULE_H
#define MOON_BACKPACK_CAPSULE_H
static int bp_finite(float v){return v>=-3.402823466e38f&&v<=3.402823466e38f;}
static int bp_finite_vector(SPVec3 v){return bp_finite(v.x)&&bp_finite(v.y)&&bp_finite(v.z);}
static int bp_float(PTR method,PTR self,float *out){
    int ok=1;PTR boxed=call(method,self,0,&ok);void *value=ok&&boxed?unbox(boxed):0;
    if(!value)return 0;*out=*(float *)value;return bp_finite(*out);
}
static int bp_vector(PTR method,PTR self,SPVec3 *out){
    int ok=1;PTR boxed=call(method,self,0,&ok);void *value=ok&&boxed?unbox(boxed):0;
    if(!value)return 0;su_copy(out,value,sizeof(*out));return bp_finite_vector(*out);
}
static int bp_destroy(PTR object){
    if(!object)return 1;int ok=1;void *args[]={object};call(su.obj_destroy,0,args,&ok);if(!ok){bp.cleanup_failed=1;bp_suspend("Capsule resource cleanup unconfirmed");}return ok;
}
static SURoot bp_material(SUColor color){
    int ok=1;PTR shader=0;void *find[]={su.string("Universal Render Pipeline/Unlit")};
    /* Keep the same fallback shaders as the existing original capsule. */
    shader=call(su.shader_find,0,find,&ok);
    if(!ok||!shader){ok=1;find[0]=su.string("Sprites/Default");shader=call(su.shader_find,0,find,&ok);}
    SURoot shader_root=su_hold(shader);if(!ok||!shader_root){su_drop(&shader_root);return 0;}
    PTR raw=su_new("UnityEngine","Material");SURoot material=su_hold(raw);
    if(!material){bp_destroy(raw);su_drop(&shader_root);return 0;}
    void *ctor[]={su_get(shader_root)},*set[]={&color};
    call(su.material_ctor,su_get(material),ctor,&ok);if(ok)call(su.material_color,su_get(material),set,&ok);
    su_drop(&shader_root);
    if(!ok){bp_destroy(su_get(material));su_drop(&material);return 0;}
    return material;
}
static int bp_capsule_materials(void){
    if(bp.orange&&bp.capsule_mat)return 1;
    SURoot orange=bp_material((SUColor){.88f,.45f,.10f,1});
    SURoot capsule=orange?bp_material((SUColor){.35f,.67f,.79f,1}):0;
    if(!orange||!capsule){bp_destroy(su_get(orange));bp_destroy(su_get(capsule));su_drop(&orange);su_drop(&capsule);return 0;}
    bp.orange=orange;bp.capsule_mat=capsule;return 1;
}
static int bp_capsule_part(PTR parent,SPVec3 position,SPVec3 scale,PTR material){
    int ok=1,shape=3,layer=2;unsigned char disabled=0,keep_world=0;
    void *create[]={&shape};PTR raw=call(su.primitive,0,create,&ok);SURoot root=su_hold(raw);
    if(!ok||!root){bp_destroy(raw);su_drop(&root);return 0;}
    PTR go=su_get(root),transform=call(su.go_transform,go,0,&ok);
    PTR collider=su_component(go,"Collider"),renderer=su_component(go,"Renderer");
    PTR set_layer=su_method("UnityEngine","GameObject","set_layer","System.Int32");
    if(!transform||!collider||!renderer||!set_layer)ok=0;
    void *off[]={&disabled},*layer_args[]={&layer},*parent_args[]={parent,&keep_world};
    void *at[]={&position},*size[]={&scale};
    if(ok)call(su.collider_enabled,collider,off,&ok);
    if(ok)call(set_layer,go,layer_args,&ok);
    if(ok)call(su.tr_parent,transform,parent_args,&ok);
    if(ok)call(su.tr_local_position,transform,at,&ok);
    if(ok)call(su.tr_scale,transform,size,&ok);
    PTR array=ok?su.new_array(su.material_class,1):0;SURoot materials=su_hold(array);
    if(!materials)ok=0;
    if(ok){PTR *items=su.array_address(su_get(materials),8,0);if(!items)ok=0;else tt.write_barrier(su_get(materials),items,material);}
    void *assign[]={su_get(materials)};if(ok)call(su.renderer_materials,renderer,assign,&ok);
    su_drop(&materials);
    if(!ok&&!bp_destroy(go))bp_suspend("Capsule visual cleanup unconfirmed");
    su_drop(&root);return ok;
}
static PTR bp_capsule_create(void){
    if(!bp_capsule_materials())return 0;
    PTR raw=su_new("UnityEngine","GameObject");bp.capsule=su_hold(raw);
    if(!bp.capsule){if(!bp_destroy(raw))bp_suspend("Capsule cleanup unconfirmed");return 0;}
    int ok=1;PTR go=su_get(bp.capsule);void *name[]={su.string("Backpack launched dust capsule")};
    if(!name[0])ok=0;else call(su.go_ctor,go,name,&ok);
    PTR type=ok?su_type("MoonCleaners","MoonLaunchCapsule"):0;void *add[]={type};
    PTR capsule=type?call(su.go_add,go,add,&ok):0;
    PTR transform=ok?call(su.go_transform,go,0,&ok):0;
    if(!ok||!capsule||!transform)return 0;
    if(!bp_capsule_part(transform,su_v(0,0,0),su_v(.35f,.7f,.35f),su_get(bp.capsule_mat))||
       !bp_capsule_part(transform,su_v(0,-.38f,0),su_v(.23f,.12f,.23f),su_get(bp.orange)))return 0;
    return capsule;
}
static int bp_capsule_discard(void){
    if(!bp_destroy(su_get(bp.capsule))){bp_suspend("Capsule cleanup unconfirmed");return 0;}
    su_drop(&bp.capsule);return 1;
}
static int bp_capsule_release(void){
    /* Preserve roots and materials when a possibly flying object survives a
     * destroy exception. Cleanup uncertainty stays latched for this DLL load,
     * including when the next world happens to retry destruction successfully. */
    if(!bp_capsule_discard())return 0;
    int orange_ok=bp_destroy(su_get(bp.orange)),capsule_ok=bp_destroy(su_get(bp.capsule_mat));
    if(orange_ok)su_drop(&bp.orange);if(capsule_ok)su_drop(&bp.capsule_mat);
    if(orange_ok&&capsule_ok&&!bp.cleanup_failed)bp.suspended=0;
    return orange_ok&&capsule_ok&&!bp.cleanup_failed;
}
static void bp_launch(PTR walker,PTR world){
    if(bp.retry>0||bp.kilograms<19.999||!bp.registered||!bp_authorized(world))return;
    bp.retry=2;int ok=1,catcher=-1,reserved=0,attempted=0;float arrival=0,now=0;
    SURoot owner=0;SPVec3 up,position,from,target={0},target_up={0};
    PTR transform=tt_transform(walker);
    if(!transform||!bp_vector(su.tr_up,transform,&up)||!bp_vector(su.tr_get_position,transform,&position))return;
    float length=su_dot(up,up);if(!(length>.5f&&length<1.5f))return;
    from=su_add(position,su_mul(up,2.1f));if(!bp_finite_vector(from))return;
    void *find[]={&from,&catcher,&target,&target_up};PTR result=call(bp.find_catcher,world,find,&ok);
    void *found=ok&&result?unbox(result):0;
    if(!found||!*(unsigned char *)found||catcher<0||!bp_finite_vector(target)||!bp_finite_vector(target_up)){
        tt.status="Capsule full: waiting for a compatible catcher";return;
    }
    void *time_args[]={&from,&target};PTR duration=call(bp.flight,0,time_args,&ok);
    void *seconds=ok&&duration?unbox(duration):0;
    if(!seconds)return;float flight=*(float *)seconds;
    if(!bp_finite(flight)||!(flight>0)||!bp_float(bp.time,0,&now)||now<0)return;
    arrival=now+flight;if(!bp_finite(arrival)||!(arrival>now))return;
    PTR player=call(tt.player,0,0,&ok);char identity[128];
    if(!ok||!tt_identity(player,identity,sizeof(identity))||!su_eq(identity,bp.owner)){
        bp_suspend("Storage player identity changed");return;
    }
    owner=su_hold(player);if(!owner)return;
    PTR capsule=bp_capsule_create();
    if(!capsule||bp.suspended)goto refused;
    /* Recheck storage after preparation and before reserving/mutating. A stale
     * cached full mass must never produce a larger capsule than its payload. */
    double dust=bp_mass(world,&ok);
    if(!ok){bp_suspend("Capsule payload could not be confirmed");goto refused;}
    bp.kilograms=dust;if(dust<19.999)goto refused;
    if(!bp_authorized(world))goto refused;
    void *reserve[]={&catcher,&arrival};PTR booked=call(bp.reserve,world,reserve,&ok);
    void *booking=ok&&booked?unbox(booked):0;
    if(!booking){bp_suspend("Catcher reservation unconfirmed");goto refused;}
    float booked_time=*(float *)booking;
    if(!bp_finite(booked_time)||booked_time<arrival){bp_suspend("Catcher reservation invalid");goto refused;}
    arrival=booked_time;reserved=1;
    if(!bp_authorized(world))goto refused;
    /* Keep the established native Launch -> Take order. If either call is
     * ambiguous, stop all further cleaning/launches in this world. There is no
     * verified native refund API and guessing one would risk duplication. */
    void *args[]={world,&catcher,&from,&up,&arrival,su_get(owner),&dust};attempted=1;
    call(bp.launch,capsule,args,&ok);
    if(!ok){bp_suspend("Capsule launch unconfirmed");goto refused;}
    if(!bp_authorized(world))goto refused;
    void *withdraw[]={su_get(bp.storage),&dust};PTR withdrawn=call(bp.take,world,withdraw,&ok);
    void *value=ok&&withdrawn?unbox(withdrawn):0;
    if(!value||*(double *)value!=dust){bp_suspend("Capsule withdrawal unconfirmed");goto refused;}
    bp.kilograms=bp_mass(world,&ok);
    if(!ok||bp.kilograms!=0){bp_suspend("Capsule storage balance unconfirmed");goto refused;}
    bp.shots++;su_drop(&bp.capsule);su_drop(&owner);
    if(!bp_authorized(world)){bp_suspend("Capsule authority changed");return;}
    call(bp.request_save,world,0,&ok);
    if(!ok){bp_suspend("Capsule launched; save request unconfirmed");return;}
    tt.status="Full capsule launched to catcher";tt_event("capsule-launched");return;
refused:
    if(attempted&&!bp.suspended)bp_suspend("Capsule authority changed");
    /* Do not free a reservation if its possibly flying capsule could not be
     * destroyed. A failed destruction keeps the GC root for teardown retry. */
    if(bp_capsule_discard()&&reserved){
        if(!tt_host())bp_suspend("Catcher cleanup lost host authority");
        else {int release_ok=1;void *release[]={&catcher,&arrival};call(bp.forget,world,release,&release_ok);if(!release_ok)bp_suspend("Catcher cleanup unconfirmed");}
    }
    su_drop(&owner);if(!bp.suspended)tt.status="Capsule launch refused";tt_event("launch-refused");
}
#endif
