#ifndef _PARTIO_EXCHANGE_H_
#define _PARTIO_EXCHANGE_H_

#include <Partio.h>
#include <iostream>
#include <string>
#include <vector>
#include "OpenEXR/ImathMatrix.h"
#include "OpenEXR/ImathMatrixAlgo.h"

class Partio_Exchange
{
  private:    
    Partio::ParticlesDataMutable *p_data;
    Partio::ParticleAttribute p_attr, s_attr, aim_XP_attr, up_XP_attr, idx_attr;

  public:    
    Partio_Exchange();
    Partio_Exchange(std::string file_name);
    ~Partio_Exchange();
    
    void load_bgeo(std::string file_name);
    void close_partio();
    int get_numParticles();    
    void printm(Imath::M44f matrix);    
    int get_index(int p_num);
    
    std::vector<float> get_m44f(int p_num);
};

#endif