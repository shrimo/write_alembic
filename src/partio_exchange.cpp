#include "partio_exchange.h"

using namespace std;


Partio_Exchange::Partio_Exchange(){};
Partio_Exchange::Partio_Exchange(string file_name)
{
    // cout <<"load bgeo -> "<< file_name.c_str() << "\n";
    p_data = Partio::read(file_name.c_str());
    p_data->attributeInfo("position", p_attr);
    p_data->attributeInfo("scale", s_attr);
    p_data->attributeInfo("aim_XP", aim_XP_attr);
    p_data->attributeInfo("up_XP", up_XP_attr);
    p_data->attributeInfo("index_XP", idx_attr);
}

Partio_Exchange::~Partio_Exchange()
{
    // p_data->release();
    cout << "- partio destructor" << "\n";
}
void Partio_Exchange::load_bgeo(string file_name)
{
    // cout <<"load bgeo -> "<< file_name.c_str() << "\n";
    p_data = Partio::read(file_name.c_str());
    p_data->attributeInfo("position", p_attr);
    p_data->attributeInfo("scale", s_attr);
    p_data->attributeInfo("aim_XP", aim_XP_attr);
    p_data->attributeInfo("up_XP", up_XP_attr);
    p_data->attributeInfo("index_XP", idx_attr);
}
void Partio_Exchange::close_partio()
{
    p_data->release();
}
int Partio_Exchange::get_numParticles()
{
    return p_data->numParticles();
}
void Partio_Exchange::printm(Imath::M44f matrix)
{
    for (auto i = 0; i < 4; i++)
    {
        for (auto j = 0; j < 4; j++)
        {
            cout << matrix[i][j] << ", ";
        }
    }
    cout << "\n";
}
int Partio_Exchange::get_index(int p_num)
{
    const int *bgeo_idx = p_data->data<int>(idx_attr, p_num);
    int idx = (*bgeo_idx - 5) / 100;
    // cout << typeid(idx).name()<<"\n";
    // cout << "index :"<< idx <<"\n";
    return idx;
}
vector<float> Partio_Exchange::get_m44f(int p_num)
{
    /* Matrix calculation
    the result is written to the vector container (float_out)*/
    vector<float> float_out;
    float_out.clear();

    const float *pos = p_data->data<float>(p_attr, p_num);
    const float *sca = p_data->data<float>(s_attr, p_num);
    const float *bgeo_aim_XP = p_data->data<float>(aim_XP_attr, p_num);
    const float *bgeo_up_XP = p_data->data<float>(up_XP_attr, p_num);

    Imath::V3f t(pos[0], pos[1], pos[2]);
    Imath::V3f s(sca[0], sca[1], sca[2]);

    Imath::V3f vec_aim(bgeo_aim_XP[0], bgeo_aim_XP[1], bgeo_aim_XP[2]);
    Imath::V3f vec_up(bgeo_up_XP[0], bgeo_up_XP[1], bgeo_up_XP[2]);
    Imath::V3f vec_dir(1, 0, 0);

    // Returns a matrix that rotates the "fromDir" vector 
    // so that it points towards "toDir".  You may also 
    // specify that you want the up vector to be pointing 
    // in a certain direction "upDir".

    Imath::M44f rot = IMATH_NAMESPACE::rotationMatrixWithUpDir(vec_dir, vec_aim, vec_up);

    Imath::M44f m44f;
    m44f.setTranslation(t);
    m44f.scale(s);

    Imath::M44f m44f_out = rot * m44f;

    for (auto i = 0; i < 4; i++)
    {
        for (auto j = 0; j < 4; j++)
        {
            float_out.push_back(m44f_out[i][j]);
        }
    }    
    // printm(m44f_out);
    return float_out;
}
