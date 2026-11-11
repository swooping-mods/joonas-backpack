#ifndef JOONAS_BACKPACK_MOTION_H
#define JOONAS_BACKPACK_MOTION_H
/* Rigid two-link articulation. Reach refusal prevents a visual arm from
 * cleaning terrain its brush cannot actually touch. No mesh scaling occurs. */
static inline int bp_solve(SPVec3 shoulder,SPVec3 target,SPVec3 bend,float upper,float lower,SPVec3 *elbow,SPVec3 *wrist){
    SPVec3 delta=su_sub(target,shoulder);float squared=su_dot(delta,delta),distance=squared>1e-8f?su_sqrt(squared):0;
    float maximum=upper+lower-.002f,minimum=upper>lower?upper-lower+.002f:lower-upper+.002f;
    int reached=distance>=minimum&&distance<=maximum;
    SPVec3 direction=distance>.00001f?su_mul(delta,1/distance):su_v(0,-1,0);
    if(distance>maximum)distance=maximum;
    if(distance<minimum)distance=minimum;
    *wrist=su_add(shoulder,su_mul(direction,distance));
    SPVec3 plane=su_sub(bend,su_mul(direction,su_dot(bend,direction)));
    if(su_dot(plane,plane)<.000001f){SPVec3 axis=direction.x*direction.x<.5f?su_v(1,0,0):su_v(0,0,1);plane=su_sub(axis,su_mul(direction,su_dot(axis,direction)));}
    plane=su_normal(plane);
    float along=(upper*upper-lower*lower+distance*distance)/(2*distance);
    float height=su_sqrt(upper*upper-along*along);
    *elbow=su_add(shoulder,su_add(su_mul(direction,along),su_mul(plane,height)));
    return reached;
}
static inline float bp_curve(float a,float b,float c,float d,float t){return .5f*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t);}
static inline void bp_ring(float phase,float *x,float *y){
    static const float ring[8][2]={{1,0},{.70710678f,.70710678f},{0,1},{-.70710678f,.70710678f},{-1,0},{-.70710678f,-.70710678f},{0,-1},{.70710678f,-.70710678f}};
    int index=(int)phase;float t=phase-index;index&=7;
    *x=bp_curve(ring[(index+7)&7][0],ring[index][0],ring[(index+1)&7][0],ring[(index+2)&7][0],t);
    *y=bp_curve(ring[(index+7)&7][1],ring[index][1],ring[(index+1)&7][1],ring[(index+2)&7][1],t);
}
static inline SPVec3 bp_frame(SPVec3 local,SPVec3 right,SPVec3 up,SPVec3 forward){return su_add(su_mul(right,local.x),su_add(su_mul(up,local.y),su_mul(forward,local.z)));}
static inline SPVec3 bp_probe(float phase,unsigned side,SPVec3 right,SPVec3 forward){float x,y;bp_ring(phase,&x,&y);return su_add(su_mul(right,(side?1.f:-1.f)*(.64f+.045f*x)),su_mul(forward,-.02f+.06f*y));}
#endif
