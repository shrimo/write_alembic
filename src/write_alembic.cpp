#include <Python.h>
#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathMatrixAlgo.h>

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreAbstract/All.h>
#include "partio_exchange.h"

#include <ctime>
#include <boost/config.hpp>
#include <boost/python.hpp>
#include <boost/filesystem.hpp>

#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MDataHandle.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MVectorArray.h>
#include <maya/MVector.h>
#include <maya/MDoubleArray.h>

#include <maya/MAnimControl.h>
#include <maya/MTime.h>

using namespace std;

namespace AbcGeom = Alembic::AbcGeom;
namespace Abc = Alembic::Abc;
namespace py = boost::python;
namespace fs = boost::filesystem;
#define API extern "C" BOOST_SYMBOL_EXPORT
class MASH_Point;
using AlembicDict = map<int, vector<MASH_Point>>;

const string __version__("1.1.0");
const float PI180 = 3.1415926535897932384626433832795 / 180.0;
inline float radian(float deg)
{
    return deg * PI180;
}
inline vector<float> flat_convert(Imath::M44f &input_m44f)
{
    // return from matrix44 -> float_array16
    vector<float> float_out;
    float_out.clear();

    for (auto i = 0; i < 4; i++)
    {
        for (auto j = 0; j < 4; j++)
        {
            float_out.push_back(input_m44f[i][j]);
        }
    }
    return float_out;
}

inline int py_scalar(py::object scalar)
{
	return PyLong_AsLong(PyObject_GetAttrString(scalar.ptr(), "real"));
}
class MASH_Point
{
    private:
    Imath::M44f matrix44;
    public:
	Imath::V3f position, rotation, scale;
    int obj_index;
    vector<float> float_out;
	MASH_Point(int obj_index, MVector pos, MVector rot, MVector sc)
	{
		this->obj_index = obj_index;
        for (int i = 0; i<3; i++)
        {
            position[i] = pos[i];
            rotation[i] = rot[i];
            scale[i] = sc[i];
        }
        matrix44.setTranslation(position);
        matrix44.scale(scale);
        matrix44.rotate(rotation);
        float_out = flat_convert(matrix44);
	}
	~MASH_Point()
	{
		// cout << "MASH_Point destructor\n";
	}
};

void maya_message(const string &name)
{
    MGlobal::displayInfo(MString(name.c_str()));
}

template<typename T>
ostream& operator<< (ostream& out, const vector<T>& v) {
    out << "(";
    size_t last = v.size() - 1;
    for(size_t i = 0; i < v.size(); ++i) {
        out << v[i];
        if (i != last) 
            out << ", ";
    }
    out << ")";
    return out;
}

template <typename T>
inline vector<T> py_list2stl_vector(const py::object &iterable)
{
	return vector<T>(py::stl_input_iterator<T>(iterable), py::stl_input_iterator<T>());
}
vector<string> list_directory(const fs::path &p, const string &ext)
{    
    vector<string> list_sequence;
    vector<fs::path> v;    

    if(fs::is_directory(p)) 
    {        
        copy(fs::directory_iterator(p), fs::directory_iterator(), back_inserter(v));
        sort(v.begin(), v.end());  

        for(auto& entry : v)
        {
            // std::cout << entry << "\n";
            // cout <<entry.extension()<<"\n";
            if (entry.extension() == ext)
            list_sequence.push_back(entry.string());
        }
    }
    return list_sequence;
}

bool anim_check(MString name)
{
    // get animation attribute from defaultRenderGlobals
    MObject renderGlobNode;
    MSelectionList tempList;
    tempList.add(name);
    if (tempList.length() > 0)     
        tempList.getDependNode(0, renderGlobNode);

    MFnDependencyNode fnRenderGlobals(renderGlobNode);
    MPlug animPlug = fnRenderGlobals.findPlug("animation");
    bool anim;
    animPlug.getValue(anim);
    return anim;
}

string eraseSubStr(string mainStr, const string & toErase)
{
	size_t pos = mainStr.find(toErase);
    string new_Str = mainStr;
 
	if (pos != string::npos)
	{
		new_Str.erase(pos, toErase.length());
	}
    return new_Str;
}

class Write_Alembic{
    private:
    vector<float> M_in;
    int ind_key, tsidx;
    vector<string> obj_list;

    Abc::OArchive outArchive;
    AbcGeom::OObject outTop;
    Alembic::AbcCoreAbstract::TimeSampling ts;
    AbcGeom::OXform xfrom;

    vector<AbcGeom::OPoints> points;
    vector<AbcGeom::OPointsSchema::Sample> out_psamp;
    vector<Abc::OCompoundProperty> arb_params;
    vector<AbcGeom::OPointsSchema> arb;
    vector<Abc::OFloatArrayProperty> arb_matrix;
    vector<Abc::OInt32ArrayProperty> arb_index;

    vector<vector<Imath::V3f>> positions;
    vector<vector<Alembic::Util::uint64_t>> iids;
    vector<vector<Alembic::Util::float32_t>> mcontainer;
    vector<vector<Alembic::Util::int32_t>> idx;

    Partio_Exchange exchange;

    public:
    Write_Alembic(string abc_out_file, string abc_root)
    {
        string app = "AMG bgeo2abc converter";
        string user = "v.lavrentev";
        Alembic::AbcCoreAbstract::MetaData md;
        md.set("version", __version__);

        cout <<"abc file: "<< abc_out_file << "\n";
        outArchive = Abc::CreateArchiveWithInfo(
            Alembic::AbcCoreOgawa::WriteArchive(), 
            abc_out_file.c_str(), app, user, md);
        outTop = outArchive.getTop();
        ts = Alembic::AbcCoreAbstract::TimeSampling(1 / 24.0, 1 / 24.0);
        tsidx = outTop.getArchive().addTimeSampling(ts);
        xfrom = AbcGeom::OXform(outTop, abc_root.c_str());
    }
    void make_abc_tree(vector<string> _obj_list)
    {
        obj_list = _obj_list;
        for (int i = 0; i<obj_list.size(); i++)
        {
            positions.push_back( vector<Imath::V3f>() );
            iids.push_back( vector<Alembic::Util::uint64_t>() );
            mcontainer.push_back( vector<Alembic::Util::float32_t>() );
            idx.push_back( vector<Alembic::Util::int32_t>() );

            points.push_back(AbcGeom::OPoints(xfrom, obj_list[i].c_str(), tsidx));        

            arb.push_back(points[i].getSchema());
            arb_params.push_back(arb[i].getArbGeomParams());

            arb_matrix.push_back(Abc::OFloatArrayProperty(arb_params[i], "Matrix4", tsidx) );
            arb_index.push_back(Abc::OInt32ArrayProperty(arb_params[i], "index", tsidx) );
            out_psamp.push_back(AbcGeom::OPointsSchema::Sample());
        }
    }
    void clear_vector()
    {       
        for (int i = 0; i<obj_list.size(); i++)
        {
            positions[i].clear();
            iids[i].clear();
            mcontainer[i].clear();
            idx[i].clear();
        }
        exchange.close_partio();
    }
    void clear_mash()
    {       
        for (int i = 0; i<obj_list.size(); i++)
        {
            positions[i].clear();
            iids[i].clear();
            mcontainer[i].clear();
            idx[i].clear();
        }     
    }
    void get_partio_data(string bgeo_in_file)
    {                
        cout << bgeo_in_file << "\n";        
        exchange.load_bgeo(bgeo_in_file);
        int p_num = exchange.get_numParticles();
        for (int i = 0; i<p_num; i++)
        {
            ind_key = exchange.get_index(i);            
            M_in = exchange.get_m44f(i);
            // cout <<"id :" << ind_key << "\n";
            // cout <<"m44f :" << M_in << "\n";
            iids[ind_key].push_back(i);
            positions[ind_key].push_back(Imath::V3f(M_in[12], M_in[13], M_in[14]));
            idx[ind_key].push_back(ind_key);

            // cout << positions[ind_key].size() << "\n";

            for (auto &mtx : M_in){
                mcontainer[ind_key].push_back(mtx);
            }
        }
    }
    void get_mash_data(AlembicDict abc_dict)
    {        
        for (auto &obj: abc_dict)
        {
            ind_key = obj.second[0].obj_index;                    
            iids[ind_key].push_back(obj.first);
            positions[ind_key].push_back(obj.second[0].position);
            idx[ind_key].push_back(ind_key);
            
            for (auto &mtx : obj.second[0].float_out)
            {
                mcontainer[ind_key].push_back(mtx);
            }            
        }
    }
    void write_abc()
    {
        for (int i = 0; i<obj_list.size(); i++){        
            out_psamp[i].setIds(iids[i]);
            out_psamp[i].setPositions(positions[i]);
            arb_matrix[i].set(mcontainer[i]);
            arb_index[i].set(idx[i]);
            points[i].getSchema().set(out_psamp[i]);
        }
    }
    ~Write_Alembic()
    {
        cout << "- alembic destructor" << "\n";
    }
};

API py::object bgeo2abc(py::tuple args, py::dict kw)
{
    cout<<"version: "<< __version__<<"\n";
    string bgeo_dir_path = py::extract<string>(args[0]);
    string abc_path = py::extract<string>(args[1]);
    string abc_root = py::extract<string>(args[2]);
    cout << "bgeo dir path: "<< bgeo_dir_path <<"\n";
    vector<string> bgeo_list_path = list_directory(bgeo_dir_path.c_str(), ".bgeo");
    vector<string> obj_list = py_list2stl_vector<string>(py::extract<py::list>(args[3]));  

    clock_t begin = clock();

    Write_Alembic wa(abc_path, abc_root);
    wa.make_abc_tree(obj_list);
    for (auto &bgeo_file: bgeo_list_path)
    {            
        wa.get_partio_data(bgeo_file);
        wa.write_abc();
        wa.clear_vector();
    }
	
    clock_t end = clock();
	double sec = double(end - begin) / CLOCKS_PER_SEC;
	cout <<"\ntime (second): " << sec <<"\n";

    py::list out;
    out.append("-> Successfull write .abc");
    return out;
}

API py::object mash2abc(py::tuple args, py::dict kw)
{
    maya_message(__version__);
    string mash_node = py::extract<string>(args[0]);
    string name_root = eraseSubStr(mash_node, "MASH_");
    // maya_message(name_root);
    string abc_path = py::extract<string>(args[1]);
    vector<string> obj_list = py_list2stl_vector<string>(py::extract<py::list>(args[2]));
    Write_Alembic wa(abc_path, name_root);
    wa.make_abc_tree(obj_list);
    
    MTime m_time;
    MTime sceneStartFrame = MAnimControl::minTime();
    MTime sceneEndFrame = MAnimControl::maxTime();

    int start_frame = (int)sceneStartFrame.as(MTime::uiUnit());
    int end_frame;
    //Animation attribute check 
    bool anim = anim_check("defaultRenderGlobals");
    if (anim){
        maya_message("Animation on");
        end_frame = (int)sceneEndFrame.as(MTime::uiUnit());
    }else{
        maya_message("Animation off");
        end_frame = (int)sceneStartFrame.as(MTime::uiUnit());
    }
                
    maya_message("Start Frame: " + to_string(start_frame));
    maya_message("End Frame: " + to_string(end_frame));
        
    for (int frame = start_frame; frame <= end_frame; frame++)
    {
        m_time.setValue((double)frame);
        MAnimControl::setCurrentTime(m_time);

        MSelectionList sel;
        sel.add(mash_node.c_str());
        MObject mash_obj;
        sel.getDependNode(0, mash_obj);
        MFnDependencyNode mashNodeFn(mash_obj);
        MObject pointsAttribute = mashNodeFn.attribute("inputPoints");
        MPlug pointsPlug(mash_obj, pointsAttribute);
        MObject handleData = pointsPlug.asMDataHandle().data();
        MFnArrayAttrsData inputPointsData(handleData);

        MVectorArray position_pp = inputPointsData.getVectorData("position");
        MVectorArray rotation_pp = inputPointsData.getVectorData("rotation");
        MVectorArray scale_pp = inputPointsData.getVectorData("scale");
        MDoubleArray index_pp = inputPointsData.getDoubleData("objectIndex");

        unsigned int array_size = position_pp.length();    
        maya_message("frame:" + to_string(frame));
        float t[3], r[3], s[3];
        AlembicDict abc_dict;

        for (int i = 0; i<array_size; i++)
        {
            int index = (int)index_pp[i];

            MASH_Point m_point(index, position_pp[i], rotation_pp[i], scale_pp[i]);
            abc_dict[i].push_back(m_point);     
        }
        
        wa.get_mash_data(abc_dict);
        wa.write_abc();
        wa.clear_mash();
    }

    m_time.setValue(start_frame);
    MAnimControl::setCurrentTime(m_time);
    py::list out;
    out.append("-> Successfull write");
    out.append(abc_path);
    return out;
}

API py::object maya_play(py::tuple args, py::dict kw)
{
    int frame = py_scalar(args[0]);
    maya_message(to_string(frame));
    
    short anim = anim_check("defaultRenderGlobals");
            
    if (anim)
    {
        maya_message("True");
    }
    else
    {
        maya_message("False");
    }
    // maya_message(to_string(anim));

    MTime m_time;
    m_time.setValue((double)frame);
    MAnimControl::setCurrentTime(m_time);

    py::list out;
    out.append("-> CurrentTime");    
    return out;
}


