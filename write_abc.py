# from maya import cmds
import time
import maya.cmds as cmds
import MASH.api as mapi
import sys
modules = '/scripts'
if amg_modules not in sys.path:
    sys.path.append(amg_modules)
from amg.system import cpp

def abc_main():
    # import os
    # print "ver 0.13"
    # path_lib = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'symmetry_check.so')
    # path_lib2 = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'dll.so')

    # shapes = cmds.ls(selection=True, shapes=True, dagObjects=True)
    # print shapes

    # cmds.select(clear=True)    
   
    # shapes_list = []
    # for shape in shapes:        
        # shapes_list.append(str(shape))

    mash_nodes_x = cmds.ls(type=['MASH_Waiter'], selection=True) or cmds.ls(type=['MASH_Waiter'])
    if not mash_nodes_x:
        print 'No MASH nodes found'
        return
    mash_nodes = str(mash_nodes_x[0])
    print 'MASH node: ', mash_nodes
    # mash_nodes = 'MASH1'
    # mash_nodes = 'Boxes'
    abc_path = '/home/v.lavrentev/projects/rnd/write_alembic/out_abc.abc'
    bgeo_dir_path = '/home/v.lavrentev/projects/cpp/partio/bgeo/01'
    obj_list = ['source', 'tree', 'bushes', 'grass']

    mashNetwork = mapi.Network(mash_nodes)
    node_ins = mashNetwork.instancer
    if node_ins.lower().endswith('_instancer'):
            input_name = '{node}.inputHierarchy'.format(node=node_ins)
            input_template = input_name + '[{index}]'
    else:
        input_name = '{node}.instancedGroup'.format(node=node_ins)
        input_template = input_name + '[{index}].instancedMesh[0].mesh'
    instance_indices = sorted(cmds.getAttr(input_name, multiIndices=True))
    instance_list = []
    for i in instance_indices:
        instance_list.append('asset_name_' + str(i))

    print instance_list
    # print 'animation: ', cmds.getAttr("defaultRenderGlobals.animation")
    
    start_C_time = time.time()
    # bgeo_cpp = cpp.call(bgeo_dir_path, abc_path, obj_list, lib='write_alembic', func="bgeo2abc")
    mash_cpp = cpp.call(mash_nodes, abc_path, instance_list, lib='write_alembic', func="mash2abc")
    # frame = 5
    # cmds.currentTime(25)
    # mash_cpp = cpp.call(frame, lib='write_alembic', func="maya_play")
    end_C_time = time.time()
 
    print mash_cpp[0]
    print mash_cpp[1]
    print("--- %s C++ time ---" % (end_C_time  - start_C_time))
    
    
    # print non_symmetrical

if __name__ == '__main__':
    abc_main()
