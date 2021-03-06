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

///@file constitutive.hpp
///@brief solver: solve the mechanical thermomechanical equilibrium			//
//	for a homogeneous loading path, allowing repeatable steps
///@version 1.9

#include <iostream>
#include <fstream>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <armadillo>
#include <smartplus/parameter.hpp>
#include <smartplus/Umat/umat_L_elastic.hpp>
#include <smartplus/Libraries/Solver/read.hpp>
#include <smartplus/Libraries/Phase/state_variables_M.hpp>
#include <smartplus/Libraries/Phase/phase_characteristics.hpp>
#include <smartplus/Libraries/Continuum_Mechanics/recovery_props.hpp>


using namespace std;
using namespace arma;
using namespace smart;

ofstream output("Props.txt");

int main() {
    
	///Material properties reading, use "material.dat" to specify parameters values
    string umat_name;
    string path_data = "data";
    string materialfile = "material.dat";
	
    int nprops = 0;
    int nstatev = 0;
    vec props;
    
    double psi_rve = 0.;
    double theta_rve = 0.;
    double phi_rve = 0.;
    
    double T_init = 273.15;

    read_matprops(umat_name, nprops, props, nstatev, psi_rve, theta_rve, phi_rve, path_data, materialfile);
    phase_characteristics rve;
    
    rve.sptr_matprops->update(0, umat_name, 1, psi_rve, theta_rve, phi_rve, props.n_elem, props);
    rve.construct(0,1);
    rve.sptr_sv_global->update(zeros(6), zeros(6), zeros(6), zeros(6), zeros(3,3), zeros(3,3), T_init, 0., nstatev, zeros(nstatev), zeros(nstatev));
    
    auto sv_M = std::dynamic_pointer_cast<state_variables_M>(rve.sptr_sv_global);

    //Second we call a recursive method that find all the elastic moduli iof the phases
    get_L_elastic(rve);
    
    string eq_UMAT;
    int eq_axis;
    vec eq_props;
    int eq_maj_sym;
    
    check_symetries(sv_M->Lt, eq_UMAT, eq_axis, eq_props, eq_maj_sym);
    
	cout << "Umat is equivalent to " << eq_UMAT << " with following props : " << endl;
    if (eq_UMAT == "ELISO") {
        cout << "E = " << eq_props(0) << " ; nu = " << eq_props(1)  << endl;
    }
    else if(eq_UMAT == "ELIST") {
        cout << " EL = " << eq_props(0) << "\n ET = " << eq_props(1) << "\n nuLT = " << eq_props(2)  << "\n nuTT = " << eq_props(3)  << "\n GLT = " << eq_props(4) << endl;
    }
    else if(eq_UMAT == "ELCUB") {
        cout << " E = " << eq_props(0) << "\n nu = " << eq_props(1) << "\n G = " << eq_props(2) << endl;
    }
    else if(eq_UMAT == "ELORT") {
        cout << " E1 = " << eq_props(0) << "\n E2 = " << eq_props(1) << "\n E3 = " << eq_props(2) << "\n nu12 = " << eq_props(3) << "\n nu13 = " << eq_props(4) << "\n nu23 = " << eq_props(5) << "\n G12 = " << eq_props(6) << "\n G13 = " << eq_props(7) << "\n G23 = " << eq_props(8) << endl;
    }
    else {
        cout << "No equivalent elastic props computed !" << endl;
	}
    
	return 0;
}
