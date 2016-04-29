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

///@file step_thermomeca.hpp
///@brief object that defines a thermomechanical step
///@version 1.0

#include <iostream>
#include <sstream>
#include <fstream>
#include <assert.h>
#include <math.h>
#include <armadillo>
#include <smartplus/parameter.hpp>
#include <smartplus/Libraries/Solver/step.hpp>
#include <smartplus/Libraries/Solver/step_thermomeca.hpp>
#include <smartplus/Libraries/Phase/state_variables_T.hpp>

using namespace std;
using namespace arma;

namespace smart{
    
//=====Private methods for ellipsoid_characteristics===================================

//=====Public methods for ellipsoid_characteristics============================================

//@brief default constructor
//-------------------------------------------------------------
step_thermomeca::step_thermomeca() : step()
//-------------------------------------------------------------
{
    cBC_meca = zeros<Col<int> >(6);
    BC_meca = zeros(6);
    BC_T = 0.;
    cBC_T = 0;
}

/*!
 \brief Constructor with parameters
 cBC_meca = mcBC_meca;  Type of boundary conditions
 BC_meca = mBC_meca;    Target values of the BC
 BC_T = mBC_T;          target temperature
 Etot = mEtot;          current strain
 sigma = msigma;        current stress
 T = mT;                current temperature
 Q = mQ                 heat
 */

//-------------------------------------------------------------
step_thermomeca::step_thermomeca(const int &mnumber, const double &mDn_init, const double &mDn_mini, const double &mDn_inc, const int &mmode, const Col<int> &mcBC_meca, const vec &mBC_meca, const mat &mmecas, const double &mBC_T, const int &mcBC_T, const vec &mTs) : step(mnumber, mDn_init, mDn_mini, mDn_inc, mmode)
//-------------------------------------------------------------
{
    cBC_meca = mcBC_meca;
    BC_meca = mBC_meca;
    mecas = mmecas;
    BC_T = mBC_T;
    cBC_T = mcBC_T;
    Ts = mTs;
}

/*!
 \brief Copy constructor
 \param bl block object to duplicate
 */

//------------------------------------------------------
step_thermomeca::step_thermomeca(const step_thermomeca& stm) : step(stm)
//------------------------------------------------------
{
    cBC_meca = stm.cBC_meca;
    BC_meca = stm.BC_meca;
    mecas = stm.mecas;
    BC_T = stm.BC_T;
    cBC_T = stm.cBC_T;
    Ts = stm.Ts;
}

/*!
 \brief destructor
 */

step_thermomeca::~step_thermomeca() {}

//-------------------------------------------------------------
void step_thermomeca::generate(const double &mTime, const vec &msigma, const vec &mEtot, const double &mT)
//-------------------------------------------------------------
{
    
    //This in for the case of an incremental path file, to get the number of increments
    string buffer;
    ifstream pathinc;
    if(mode == 3){
        ninc = 0;
        pathinc.open(file, ios::in);
        if(!pathinc)
        {
            cout << "Error: cannot open the file << " << file << "\n Please check if the file is correct and is you have added the extension\n";
        }
        //read the file to get the number of increments
        while (!pathinc.eof())
        {
            getline (pathinc,buffer);
            if (buffer != "") {
                ninc++;
            }
        }
        pathinc.close();
    }
    
    step::generate();
    
    Ts = zeros(ninc);
    mecas = zeros(ninc, 6);
    
    vec inc_coef = ones(ninc);
    if (mode == 2) {
        double sum_ = 0.;
        for(int k = 0 ; k < ninc ; k++){
            inc_coef(k) =  cos(pi + (k+1)*2.*pi/(ninc+1))+1.;
            sum_ += inc_coef(k);
        }
        inc_coef = inc_coef*ninc/sum_;
    }
    
    if (mode < 3) {
        for (int i=0; i<ninc; i++) {
            times(i) = (BC_Time)/ninc;
            
            for(int k = 0 ; k < 6 ; k++) {
                if (cBC_meca(k) == 1){
                    mecas(i,k) = inc_coef(i)*(BC_meca(k)-msigma(k))/ninc;
                }
                else if (cBC_meca(k) == 0){
                    mecas(i,k) = inc_coef(i)*(BC_meca(k)-mEtot(k))/ninc;
                }
            }
            
            if (cBC_T == 1) {
                Ts(i) = BC_T;                   //Note that here it is the flux that is imposed
            }
            else if(cBC_T == 0) {
                Ts(i) = inc_coef(i)*(BC_T - mT)/ninc;
            }
            
        }
    }
    else if (mode ==3){ ///Incremental loading
        
        //Look at how many cBc are present to know the size of the file (1 for time + 6 for each meca + 1 for temperature):
        int size_BC = 8;
        for(int k = 0 ; k < 6 ; k++) {
            if (cBC_meca(k) == 2){
                size_BC--;
            }
        }
        if (cBC_T > 2 ) {
            size_BC--;
        }
        
        vec BC_file_n = zeros(size_BC); //vector that temporarly stores the previous values
        vec BC_file = zeros(size_BC); //vector that temporarly stores the values
        
        BC_file_n(0) = mTime;
        int kT = 0;
        if (cBC_T == 0) {
            BC_file_n(kT+1) = mT;
            kT++;
        }
        else if (cBC_T == 1) {
            BC_file_n(kT+1) = 0.;    //Heat flux does not depend on any previous condition
            kT++;
        }
        
        for (int k=0; k<6; k++) {
            if (cBC_meca(k) == 0) {
                BC_file_n(kT+1) = mEtot(k);
                kT++;
            }
            if (cBC_meca(k) == 1) {
                BC_file_n(kT+1) = msigma(k);
                kT++;
            }
        }
                
        //Read all the informations and fill the meca accordingly
        pathinc.open(file, ios::in);
        
        for (int i=0; i<ninc; i++) {
            
            pathinc >> buffer;
            for (int j=0; j<size_BC; j++) {
                pathinc >> BC_file(j);
            }
            
            times(i) = (BC_file(0) - BC_file_n(0));
            kT = 0;
            if (cBC_T == 0) {
                Ts(i) = BC_file(kT+1) - BC_file_n(kT+1);
                kT++;
            }
            else if (cBC_T == 1) {
                Ts(i) = BC_file(kT+1);  //Case of Heat, direct quantity
                kT++;
            }
            else if(cBC_T == 2) {
                Ts(i) = 0.;
            }
            
            for(int k = 0 ; k < 6 ; k++) {
                if (cBC_meca(k) < 2){
                    mecas(i,k) = BC_file(kT+1) - BC_file_n(kT+1);
                    kT++;
                }
                else if (cBC_meca(k) == 2){
                    mecas(i,k) = 0.;
                }
            }
            BC_file_n = BC_file;
        }
        //At the end, everything static becomes a stress-controlled with zeros
        for(int k = 0 ; k < 6 ; k++) {
            if (cBC_meca(k) == 2)
                cBC_meca(k) = 1;
        }
        //And everything thermally static is an isothermal path
        if (cBC_T == 2) {
            cBC_T = 0;
        }
                
	}
	else {
		cout << "\nError: The mode of the step number " << number << " does not correspond to an existing loading mode.\n";
	}
    
}
    
/*!
 \brief Standard operator = for block
 */

//----------------------------------------------------------------------
step_thermomeca& step_thermomeca::operator = (const step_thermomeca& stm)
//----------------------------------------------------------------------
{
//	assert(stm.ninc>0);
//	assert(stm.mode>0);
    
	number = stm.number;
	ninc = stm.ninc;
	mode = stm.mode;
    
    BC_Time = stm.BC_Time;
    
    cBC_meca = stm.cBC_meca;
    BC_meca = stm.BC_meca;
    BC_T = stm.BC_T;
    cBC_T = stm.cBC_T;    
    
	return *this;
}

void step_thermomeca::output(ostream& output, const solver_output &so, const int &kblock, const int&kcycle, const int &kinc, const state_variables_T &sv_T) {
    
    output << kblock+1 << "\t";
    output << kcycle+1 << "\t";
    output << number+1 << "\t";
    output << kinc+1 << "\t";
    output << times(kinc) << "\t\t";
    
    if (so.o_nb_T) {
        output << sv_T.T  << "\t";
        output << 0 << "\t";                //This is for the flux
    }
    if (so.o_nb_meca) {
        for (int z=0; z<so.o_nb_meca; z++) {
            output << sv_T.Etot(so.o_meca(z)) << "\t";
        }
        for (int z=0; z<so.o_nb_meca; z++) {
            output << sv_T.sigma(so.o_meca(z)) << "\t";
        }
    }
    
    output << "\t";
    if(so.o_nw_statev != 0){
        if (so.o_wanted_statev(0) < 0) {
            for(int k = 0 ; k < sv_T.nstatev ; k++)
            output << sv_T.statev(k) << "\t";
        }
        else{
            for(int k = 0 ; k < so.o_nw_statev ; k++){
                for (int l = so.o_wanted_statev(k); l < (so.o_range_statev(k)+1); l++){
                    output << sv_T.statev(l) << "\t";
                }
            }
        }
    }
    output << endl;
    
}
    
//--------------------------------------------------------------------------
ostream& operator << (ostream& s, const step_thermomeca& stm)
//--------------------------------------------------------------------------
{
    s << "\tDisplay info on the step " << stm.number << "\n";
    s << "\tLoading mode: " << stm.mode << "\n";
    s << "\tThermomechanical step \n\t";
    
    Col<int> temp;
    temp = zeros<Col<int> >(6);
    temp(0) = 0;
	temp(1) = 3;
	temp(2) = 1;
	temp(3) = 4;
	temp(4) = 5;
	temp(5) = 2;
    
    if (stm.mode == 3) {
        for(int k = 0 ; k < 6 ; k++) {
            if(stm.cBC_meca(temp(k)) == 0)
                s << "E " << (((k==0)||(k==2)||(k==5)) ? "\n\t" : "\t");
            else if(stm.cBC_meca(temp(k)) == 1)
                s << "S " << (((k==0)||(k==2)||(k==5)) ? "\n\t" : "\t");
            else if(stm.cBC_meca(temp(k)) == 2)
                s << 0 << (((k==0)||(k==2)||(k==5)) ? "\n\t" : "\t");
        }
        cout << "Temperature: ";
        if(stm.cBC_T == 0)
            s << "T\n";
        else if(stm.cBC_T == 1)
            s << "Q\n";
        else if(stm.cBC_T == 2)
            s << 0 << "\n";
        else if(stm.cBC_T == 3)
            s << "convexion with Tau = " << stm.BC_T << "\n";
    }
    else {
        
        s << "\tTime of the step " << stm.BC_Time << " s\n\t";
        s << "\tInitial fraction: " << stm.Dn_init << "\tMinimal fraction: " << stm.Dn_mini << "\tIncrement fraction: " << stm.Dn_inc << "\n\t";

        for(int k = 0 ; k < 6 ; k++) {
            s << ((stm.cBC_meca(temp(k)) == 0) ? "\tE " : "\tS ") << stm.BC_meca(temp(k)) << (((k==0)||(k==2)||(k==5)) ? "\n\t" : "\t");
        }
        s << "Temperature at the end of step: " << stm.BC_T << "\n";
    }
	return s;
}

} //namespace smart
