/* Backpack-local storage and progression admission. Shared wallet/ownership
 * behavior remains in the equipment provider adapter. */
#ifndef MOON_BACKPACK_STATE_H
#define MOON_BACKPACK_STATE_H
static void bp_suspend(const char *status){
    bp.suspended=1;bp.contact[0]=bp.contact[1]=0;tt.status=status;tt_event("storage-suspended");
}
static int bp_authorized(PTR world){
    return cfg.enabled&&tt.owned&&!tt.invalid_state&&!bp.suspended&&world&&world==su_get(tt.world)&&tt_host();
}
static int feature_can_buy(PTR world){
    int ok=1;PTR player=call(tt.player,0,0,&ok);SURoot owner=su_hold(player);
    if(!ok||!owner){su_drop(&owner);return 0;}
    void *args[]={su_get(owner)};PTR owned=call(bp.levels,world,args,&ok);
    int levels=0,total=0;void *value=ok&&owned?unbox(owned):0;
    if(value)levels=*(int *)value;else ok=0;
    PTR count=ok?call(bp.total,world,0,&ok):0;value=ok&&count?unbox(count):0;
    if(value)total=*(int *)value;else ok=0;su_drop(&owner);
    /* ceil(total/2) avoids signed overflow in levels*2 or total+1. */
    if(!ok||total<1||levels<12||levels>total||levels<total/2+total%2){
        tt.status="Late game: own at least 12 and half of all upgrade levels";return 0;
    }
    return 1;
}
static void bp_register(PTR world){
    if(bp.registered||!bp_authorized(world))return;
    int ok=1;char id[128],player_id[128];PTR player=call(tt.player,0,0,&ok);
    if(!ok||!tt_identity(player,player_id,sizeof(player_id))){bp_suspend("Storage player identity unavailable");return;}
    unsigned n=0;while(player_id[n])n++;
    char *p=cat(id,"moonlab.joonas-backpack.");num(p,tt_hash((unsigned char *)player_id,n));
    bp.storage=su_hold(su.string(id));
    if(!bp.storage){tt.status="Storage identity could not be rooted";return;}
    double capacity=20;void *args[]={su_get(bp.storage),&capacity};
    call(bp.register_storage,world,args,&ok);
    if(!ok){su_drop(&bp.storage);bp_suspend("Storage registration unconfirmed");return;}
    cat(bp.owner,player_id);bp.registered=1;tt_event("storage-registered");
}
static double bp_mass(PTR world,int *ok){
    if(!*ok||!bp.registered||!su_get(bp.storage)){*ok=0;return 0;}
    void *args[]={su_get(bp.storage)};PTR kg=call(bp.storage_mass,world,args,ok);
    void *value=*ok&&kg?unbox(kg):0;
    if(!value){*ok=0;return 0;}
    double mass=*(double *)value;
    /* Ordered comparisons also refuse NaN and either infinity. */
    if(!(mass>=0&&mass<=20)){*ok=0;return 0;}
    return mass;
}
static int bp_clean_stroke(PTR world,void **args){
    if(!bp_authorized(world)||bp.kilograms>=19.999)return 0;
    SURoot player=su_hold(args[9]);char identity[128];
    if(!player||!tt_identity(su_get(player),identity,sizeof(identity))||!su_eq(identity,bp.owner)){
        su_drop(&player);bp_suspend("Cleaning player identity unavailable");return 0;
    }
    args[9]=su_get(player);int ok=1;call(bp.clean,world,args,&ok);su_drop(&player);
    if(!ok){bp_suspend("Cleaning result unconfirmed");return 0;}
    bp.kilograms=bp_mass(world,&ok);
    if(!ok){bp_suspend("Cleaning storage balance unconfirmed");return 0;}
    return 1;
}
#endif
