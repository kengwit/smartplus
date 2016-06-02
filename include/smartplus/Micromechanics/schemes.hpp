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

///@file schemes.hpp
///@brief micromechanical schemes for non-linear N-phases heterogeneous materials:
///@brief Mori-Tanaka scheme 
///@version 1.0

#pragma once

#include <armadillo>
#include <smartplus/Libraries/Phase/phase_characteristics.hpp>

namespace smart{

// The Mori-Tanaka function works like a UMAT with the following material properties
///@brief props[0] : Number of phases
///@brief props[1] : Number of the file NPhase[i].dat utilized
///@brief props[2] : Number of integration points in the 1 direction
///@brief props[3] : Number of integration points in the 2 direction

void Lt_Homogeneous_E(phase_characteristics &);
void DE_Homogeneous_E(phase_characteristics &);

void Lt_Mori_Tanaka(phase_characteristics &);
void DE_Mori_Tanaka(phase_characteristics &);
    
void Lt_Self_Consistent(phase_characteristics &, const bool &, const int &);
void DE_Self_Consistent(phase_characteristics &, const bool &, const int &);

void Lt_Periodic_Layer(phase_characteristics &);
void dE_Periodic_Layer(phase_characteristics &, const int &);
    
} //namespace smart
