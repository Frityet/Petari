"""Read-only authored-to-world approach candidates; no game state or inputs are written."""
import json,math,pathlib,struct,hashlib
base=pathlib.Path(__file__).parent
f=lambda x:struct.unpack('f',struct.pack('f',x))[0]
def mul(a,b):return [[f(sum(a[i][k]*b[k][j] for k in range(3))) for j in range(3)] for i in range(3)]
def point(m,p,t):return [f(sum(m[i][k]*p[k] for k in range(3))+t[i]) for i in range(3)]
def vec(r,p):return [r[p+'_'+k] for k in 'xyz']
def rotate(degrees,short=False):
 vals=[];indices=[]
 for d in degrees:
  if short:
   idx=(int(f(f(d)*f(182.04445)))&65535)>>2;indices.append(idx)
   angle=idx*6.2831854820251465/16384
  else:angle=f(f(d)*f(f(math.pi)/180))
  vals.append((f(math.sin(angle)),f(math.cos(angle))))
 (sx,cx),(sy,cy),(sz,cz)=vals
 return [[f(cz*cy),f(f(f(cz*sy)*sx)-f(sz*cx)),f(f(f(cz*sy)*cx)+f(sz*sx))],
         [f(sz*cy),f(f(f(sz*sy)*sx)+f(cz*cx)),f(f(f(sz*sy)*cx)-f(cz*sx))],
         [-sy,f(cy*sx),f(cy*cx)]],indices
def euler(m):
 if -.001<=m[2][0]-1:ang=[math.atan2(-m[0][1],m[1][1]),-1.5707964,0]
 elif m[2][0]+1<=.001:ang=[math.atan2(m[0][1],m[1][1]),1.5707964,0]
 else:ang=[math.atan2(m[2][1],m[2][2]),math.asin(-m[2][0]),math.atan2(m[1][0],m[0][0])]
 return [f(f(a)*f(57.29578)) for a in ang]
rawgalaxy=pathlib.Path('notes/original-switch-area-transforms-20260919/raw-galaxy.json')
zone=next(r for r in json.loads(rawgalaxy.read_text()) if r.get('name')=='HeavensDoorMysteriousZone')
zm,_=rotate(vec(zone,'dir'));zt=vec(zone,'pos')
metadata=json.loads((base/'zone-metadata.json').read_text());rows=[]
for path,entries in metadata['tables'].items():
 if '/common/' in path or '/layera/' in path:rows.extend(dict(table=path,**r) for r in entries)
result={'scope':'Read-only navigation candidates, not observed reachable paths or gameplay completion.',
 'rotation_convention':'Column vectors; Rz * Ry * Rx in degrees. Zone matrix uses native makeMtxTR float trig. Original getJMapInfoRotate composes zone with short-angle JMath local rotation, extracts Euler, then AreaFormCube uses short-angle JMath again.',
 'numerical_limit':'Python float32 rounding model with double intermediates for dot products; not a bit-exact native capture. FMA/libm rounding may differ by a few thousandths. Candidates have 75 or 225 units margin to the upper face.',
 'world_formula':'world = zone_translation + zone_rotation * local_origin + world_form_rotation * [0, 0.75*1000*scale_y, 0]',
 'assumptions':['The common stageobj row is zone5 under the restored original holder placement matrix.','Cube2 means x/z centered and y from0 to1000*scale_y; no attached external form matrix for these ordinary SwitchCube placements.','Surface accessibility is unproven; especially hole75% may be inside the crater shaft and requires walking into the opening. Use this direction for navigation, not teleportation.','Initial rabbit positions are transformed authored spawns, not their later moving pursuit positions. Select live actor by zone/type/l_id or collector group, then follow current world position.'],
 'zone_source_path':str(rawgalaxy),'zone_source_sha256':hashlib.sha256(rawgalaxy.read_bytes()).hexdigest(),'zone_row':zone,'zone_matrix_3x4':[zm[i]+[zt[i]] for i in range(3)],'switch_cubes':[],'rabbit_spawns':[],'pipes':[],'warp_pods':[]}
for r in rows:
 if r.get('name')=='SwitchCube' and r.get('SW_A') in (1113,1114):
  local_rot,local_indices=rotate(vec(r,'dir'),True);composed=mul(zm,local_rot);world_euler=euler(composed);form_rot,world_indices=rotate(world_euler,True)
  origin=point(zm,vec(r,'pos'),zt);height=f(f(1000)*r['scale_y']);p=[0,f(height*.75),0];upper=[0,f(height*.9),0]
  result['switch_cubes'].append({'label':'bush' if r['SW_A']==1113 else 'hole','raw_row':r,'world_origin':origin,'world_euler_degrees':world_euler,'local_short_indices':local_indices,'world_short_indices':world_indices,'world_form_rotation':form_rot,'local_candidate':p,'world_candidate_y75_percent':point(form_rot,p,origin),'world_candidate_y90_percent':point(form_rot,upper,origin),'form_bounds_min':[-500*r['scale_x'],0,-500*r['scale_z']],'form_bounds_max':[500*r['scale_x'],height,500*r['scale_z']]})
 elif r.get('name')=='RunawayRabbit':result['rabbit_spawns'].append({'raw_row':r,'world_spawn':point(zm,vec(r,'pos'),zt)})
 elif r.get('name')=='EarthenPipe':result['pipes'].append({'raw_row':r,'world_authored_base':point(zm,vec(r,'pos'),zt),'warning':'Actual top matrix/actor translation is raised120 along gravity-up by original calcTrans; use real runtime binder/top as the approach target.'})
 elif r.get('name')=='WarpPod':result['warp_pods'].append({'raw_row':r,'world_sensor_center':point(zm,vec(r,'pos'),zt),'sensor_radius':120*r['scale_x'] if r['Obj_arg1']==0 else 15*r['scale_x']})
(base/'controller-waypoints.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
for r in result['switch_cubes']:print(r['label'],r['world_candidate_y75_percent'], '90%',r['world_candidate_y90_percent'])
for r in result['rabbit_spawns']:print('rabbit',r['raw_row']['l_id'],'group',r['raw_row']['Obj_arg0'],r['world_spawn'])
for r in result['pipes']:print('pipe',r['raw_row']['l_id'],'writes',r['raw_row']['SW_B'],r['world_authored_base'])
