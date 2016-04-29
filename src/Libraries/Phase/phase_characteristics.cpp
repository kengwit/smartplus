/* This file is part of SMART+.
 
 SMART+ is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SMART+ is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SMART+.  If not, see <http://www.gnu.org/licenses/>.
 
 */

///@file phase_characteristics.cpp
///@brief Characteristics of a phase, which hereditates from:
///- material_characteristics
///@version 1.0

#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>
#include <armadillo>
#include <smartplus/Libraries/Phase/phase_characteristics.hpp>
#include <smartplus/Libraries/Geometry/geometry.hpp>
#include <smartplus/Libraries/Geometry/layer.hpp>
#include <smartplus/Libraries/Geometry/ellipsoid.hpp>
#include <smartplus/Libraries/Homogenization/phase_multi.hpp>
#include <smartplus/Libraries/Homogenization/layer_multi.hpp>
#include <smartplus/Libraries/Homogenization/ellipsoid_multi.hpp>
#include <smartplus/Libraries/Phase/material_characteristics.hpp>
#include <smartplus/Libraries/Phase/state_variables.hpp>
#include <smartplus/Libraries/Phase/state_variables_M.hpp>
#include <smartplus/Libraries/Phase/state_variables_T.hpp>
#include <smartplus/Libraries/Solver/output.hpp>



using namespace std;
using namespace arma;

namespace smart{

//=====Private methods for phase_characteristics===================================

//=====Public methods for phase_characteristics============================================

/*!
  \brief default constructor
*/

//-------------------------------------------------------------
    phase_characteristics::phase_characteristics()
//-------------------------------------------------------------
{
    shape_type = 0;
    sv_type = 0;
    sptr_matprops = std::make_shared<material_characteristics>();
    
    //Note : the construction of sptr_shape = std::make_shared.. and sptr_sv = std::make_shared.. is made in construct(int, int)

    
}

/*!
  \brief Constructor with parameters
  \f$ \textbf{Examples :} \f$ \n
*/

//-------------------------------------------------------------
    phase_characteristics::phase_characteristics(const int &mshape_type, const int &msv_type, const std::shared_ptr<geometry> &msptr_shape, const std::shared_ptr<phase_multi> &msptr_multi, const std::shared_ptr<material_characteristics> &msptr_matprops, const std::shared_ptr<state_variables> &msptr_sv_global, const std::shared_ptr<state_variables> &msptr_sv_local, const std::shared_ptr<std::ofstream> &msptr_out)
//-------------------------------------------------------------
{
    shape_type = mshape_type;
    sv_type = msv_type;
    sptr_shape = msptr_shape;
    sptr_multi = msptr_multi;
    sptr_matprops = msptr_matprops;
    sptr_sv_global = msptr_sv_global;
    sptr_sv_local = msptr_sv_local;
    sptr_out = msptr_out;
}

/*!
  \brief Copy constructor
  \param s phase_characteristics object to duplicate
*/
    
//------------------------------------------------------
phase_characteristics::phase_characteristics(const phase_characteristics& pc)
//------------------------------------------------------
{
    shape_type = pc.shape_type;
    sv_type = pc.sv_type;
    sptr_shape = pc.sptr_shape;
    sptr_multi = pc.sptr_multi;
    sptr_matprops = pc.sptr_matprops;
    sptr_sv_global = pc.sptr_sv_global;
    sptr_sv_local = pc.sptr_sv_local;
    sptr_out = pc.sptr_out;
}

/*!
  \brief Destructor

  Deletes phase_characteristics, the shared ptr related object will be detroyed automatically if they is no pointer to it.
*/

//-------------------------------------
phase_characteristics::~phase_characteristics() {}
//-------------------------------------

/*!
  \brief Standard operator = for phase_characteristics
*/
  
    
//-------------------------------------------------------------
void phase_characteristics::construct(const int &mshape_type, const int &msv_type)
//-------------------------------------------------------------
{
    assert(msv_type > 0);
    
    shape_type = mshape_type;
    sv_type = msv_type;
    
    //Switch case for the geometry of the phase
    switch (shape_type) {
        case 0: {
            sptr_shape = std::make_shared<geometry>();
            sptr_multi = std::make_shared<phase_multi>();
            break;
        }
        case 1: {
            sptr_shape = std::make_shared<layer>();
            sptr_multi = std::make_shared<layer_multi>();
            break;
        }
        case 2: {
            sptr_shape = std::make_shared<ellipsoid>();
            sptr_multi = std::make_shared<ellipsoid_multi>();
            break;
        }
        default: {
            cout << "error: The geometry type does not correspond (0 for general, 1 for layer, 2 for ellipsoid)\n";
            exit(0);
            break;
        }
    }
    
    //Switch case for the state_variables type of the phase
    switch (sv_type) {
        case 1: {
            sptr_sv_global = std::make_shared<state_variables_M>();
            sptr_sv_local = std::make_shared<state_variables_M>();
            break;
        }
        case 2: {
            sptr_sv_global = std::make_shared<state_variables_T>();
            sptr_sv_local = std::make_shared<state_variables_T>();
            break;
        }
        default: {
            cout << "error: The state_variable type does not correspond (1 for Mechanical, 2 for Thermomechanical)\n";
            exit(0);
            break;
        }
    }

}

//-------------------------------------------------------------
void phase_characteristics::sub_phases_construct(const int &nphases, const int &mshape_type, const int &msv_type)
//-------------------------------------------------------------
{
    assert(nphases > 0);
    assert(msv_type > 0);
    
    //First we resize the vector of sub_phases
    sub_phases.resize(nphases);

    //Second, for each component of the sub_phase vector we construct the phases
    for (int i=0; i<nphases; i++) {
        sub_phases[i].construct(mshape_type, msv_type);
    }
}
    
//----------------------------------------------------------------------
void phase_characteristics::to_start()
//----------------------------------------------------------------------
{
    
    sptr_sv_global->to_start();
    sptr_sv_local->to_start();
    sptr_multi->to_start();
    for(auto r : sub_phases) {
        r.to_start();
    }
    
}
    
//----------------------------------------------------------------------
void phase_characteristics::set_start()
//----------------------------------------------------------------------
{
    sptr_sv_global->set_start();
    sptr_sv_local->set_start();
    sptr_multi->set_start();
    for(auto r : sub_phases) {
        r.set_start();
    }
}

//-------------------------------------------------------------
void phase_characteristics::local2global()
//-------------------------------------------------------------
{
    //Switch case for the state_variables type of the phase
    switch (sv_type) {
        case 1: {
            auto sv_M_g = std::dynamic_pointer_cast<state_variables_M>(sptr_sv_global);
            auto sv_M_l = std::dynamic_pointer_cast<state_variables_M>(sptr_sv_local);
            sv_M_g->rotate_l2g(*sv_M_l, sptr_matprops->psi_mat, sptr_matprops->theta_mat, sptr_matprops->phi_mat);
            break;
        }
        case 2: {
            auto sv_T_g = std::dynamic_pointer_cast<state_variables_T>(sptr_sv_global);
            auto sv_T_l = std::dynamic_pointer_cast<state_variables_T>(sptr_sv_local);
            sv_T_g->rotate_l2g(*sv_T_l, sptr_matprops->psi_mat, sptr_matprops->theta_mat, sptr_matprops->phi_mat);
            break;
        }
        default: {
            cout << "error: The state_variable type does not correspond (1 for Mechanical, 2 for Thermomechanical)\n";
            exit(0);
            break;
        }
    }

}

//-------------------------------------------------------------
void phase_characteristics::global2local()
//-------------------------------------------------------------
{
    //Switch case for the state_variables type of the phase
    switch (sv_type) {
        case 1: {
            auto sv_M_g = std::dynamic_pointer_cast<state_variables_M>(sptr_sv_global);
            auto sv_M_l = std::dynamic_pointer_cast<state_variables_M>(sptr_sv_local);
            sv_M_l->rotate_g2l(*sv_M_g, sptr_matprops->psi_mat, sptr_matprops->theta_mat, sptr_matprops->phi_mat);
            break;
        }
        case 2: {
            auto sv_T_g = std::dynamic_pointer_cast<state_variables_T>(sptr_sv_global);
            auto sv_T_l = std::dynamic_pointer_cast<state_variables_T>(sptr_sv_local);
            sv_T_l->rotate_g2l(*sv_T_g, sptr_matprops->psi_mat, sptr_matprops->theta_mat, sptr_matprops->phi_mat);
            break;
        }
        default: {
            cout << "error: The state_variable type does not correspond (1 for Mechanical, 2 for Thermomechanical)\n";
            exit(0);
            break;
        }
    }
}
    
//----------------------------------------------------------------------
phase_characteristics& phase_characteristics::operator = (const phase_characteristics& pc)
//----------------------------------------------------------------------
{
    shape_type = pc.shape_type;
    sv_type = pc.sv_type;
    sptr_shape = pc.sptr_shape;
    sptr_matprops = pc.sptr_matprops;
    sptr_sv_global = pc.sptr_sv_global;
    sptr_sv_local = pc.sptr_sv_local;
    sptr_out = pc.sptr_out;
    
	return *this;
}

//----------------------------------------------------------------------
void phase_characteristics::define_output(const std::string &outputfile)
//----------------------------------------------------------------------
{

    std::string ext_filename = outputfile.substr(outputfile.length()-4,outputfile.length());
    std::string filename = outputfile.substr(0,outputfile.length()-4); //to remove the extension
//    if(sptr_matprops->number > 0)
        filename = filename + '-' + std::to_string(sptr_matprops->number) + ext_filename;
//    else
//        filename = filename + ext_filename;
    
    std::ofstream of_file(filename);
    sptr_out = make_shared<ofstream>(filename);
    
    for(unsigned int i=0; i<sub_phases.size(); i++) {
        sub_phases[i].define_output(filename);
    }
    
}
    
//----------------------------------------------------------------------
void phase_characteristics::output(const solver_output &so, const int &kblock, const int &kcycle, const int&kstep, const int &kinc, const double & Time, const std::string &coordsys)
//----------------------------------------------------------------------
{
    
        *sptr_out << kblock+1 << "\t";
        *sptr_out << kcycle+1 << "\t";
        *sptr_out << kstep+1 << "\t";
        *sptr_out << kinc+1 << "\t";
        *sptr_out << Time << "\t\t";
    
        //Switch case for the state_variables type of the phase
        if (so.o_nb_T) {
    
            switch (sv_type) {
                case 1: {
                    *sptr_out << sptr_sv_global->T  << "\t";
                    *sptr_out << 0 << "\t";                //This is for the flux Q
                    *sptr_out << 0 << "\t";                //This is for the rpl
                    break;
                }
                case 2: {
                    //We need to cast sv
                    std::shared_ptr<state_variables_T> sv_T = std::dynamic_pointer_cast<state_variables_T>(sptr_sv_global);
                    *sptr_out << sv_T->T  << "\t";
                    *sptr_out << sv_T->Q << "\t";                //This is for the flux
                    *sptr_out << sv_T->rpl << "\t";                //This is for the rpl
                    break;
                }
                default: {
                    cout << "error: The state_variable type does not correspond (1 for Mechanical, 2 for Thermomechanical)\n";
                    exit(0);
                    break;
                }
            }
        }
    
        if(coordsys == "global") {
    
            //output
            if (so.o_nb_meca) {
                for (int z=0; z<so.o_nb_meca; z++) {
                    *sptr_out << sptr_sv_global->Etot(so.o_meca(z)) << "\t";
                }
                for (int z=0; z<so.o_nb_meca; z++) {
                    *sptr_out << sptr_sv_global->sigma(so.o_meca(z)) << "\t";
                }
            }
        
            *sptr_out << "\t";
            if(so.o_nw_statev != 0){
                if (so.o_wanted_statev(0) < 0) {
                    for(int k = 0 ; k < sptr_sv_global->nstatev ; k++)
                    *sptr_out << sptr_sv_global->statev(k) << "\t";
                }
                else{
                    for(int k = 0 ; k < so.o_nw_statev ; k++){
                        for (int l = so.o_wanted_statev(k); l < (so.o_range_statev(k)+1); l++){
                            *sptr_out << sptr_sv_global->statev(l) << "\t";
                        }
                    }
                }
            }
        }
        else if(coordsys == "local") {

            //output
            if (so.o_nb_meca) {
                for (int z=0; z<so.o_nb_meca; z++) {
                    *sptr_out << sptr_sv_local->Etot(so.o_meca(z)) << "\t";
                }
                for (int z=0; z<so.o_nb_meca; z++) {
                    *sptr_out << sptr_sv_local->sigma(so.o_meca(z)) << "\t";
                }
            }
            
            *sptr_out << "\t";
            if(so.o_nw_statev != 0){
                if (so.o_wanted_statev(0) < 0) {
                    for(int k = 0 ; k < sptr_sv_local->nstatev ; k++)
                    *sptr_out << sptr_sv_local->statev(k) << "\t";
                }
                else{
                    for(int k = 0 ; k < so.o_nw_statev ; k++){
                        for (int l = so.o_wanted_statev(k); l < (so.o_range_statev(k)+1); l++){
                            *sptr_out << sptr_sv_local->statev(l) << "\t";
                        }
                    }
                }
            }
            
        }
        *sptr_out << endl;
    
    for(auto r : sub_phases) {
        r.output(so, kblock, kcycle, kstep, kinc, Time, "global");
    }
    
}
    
    
//--------------------------------------------------------------------------
ostream& operator << (ostream& s, const phase_characteristics& pc)
//--------------------------------------------------------------------------
{
	s << "Display phase characteristics:\n";
	s << "Display geometry characteristics:\n";
    s << *pc.sptr_shape;
	s << "Display material characteristics:\n";
    s << *pc.sptr_matprops;
    s << "Display global state variables:\n";
    s << *pc.sptr_sv_global;
    s << "Display local state variables:\n";
    s << *pc.sptr_sv_local;
    
    for(auto r : pc.sub_phases) {
        s << r;
    }
    
    s << "\n\n";

	return s;
}

} //namespace smart
