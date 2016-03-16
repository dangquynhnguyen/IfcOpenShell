/********************************************************************************
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/


#include "WavefrontObjSerializer.h"

#include "../ifcgeom/IfcGeomRenderStyles.h"

#include <boost/lexical_cast.hpp>
#include <iomanip>

bool WaveFrontOBJSerializer::ready() {
	return obj_stream.is_open() && mtl_stream.is_open();
}

void WaveFrontOBJSerializer::writeHeader() {
	obj_stream << "# File generated by IfcOpenShell " << IFCOPENSHELL_VERSION << "\n";
#ifdef WIN32
	const char dir_separator = '\\';
#else
	const char dir_separator = '/';
#endif
	std::string mtl_basename = mtl_filename;
	std::string::size_type slash = mtl_basename.find_last_of(dir_separator);
	if (slash != std::string::npos) {
		mtl_basename = mtl_basename.substr(slash+1);
	}
	obj_stream << "mtllib " << mtl_basename << "\n";
	mtl_stream << "# File generated by IfcOpenShell " << IFCOPENSHELL_VERSION << "\n";
}

void WaveFrontOBJSerializer::writeMaterial(const IfcGeom::Material& style)
{
    std::string material_name = (settings().get(IfcGeom::IteratorSettings::USE_MATERIAL_NAMES)
        ? style.original_name() : style.name());
    IfcUtil::sanitate_material_name(material_name);
    mtl_stream << "newmtl " << material_name << "\n";
	if (style.hasDiffuse()) {
		const double* diffuse = style.diffuse();
		mtl_stream << "Kd " << diffuse[0] << " " << diffuse[1] << " " << diffuse[2] << "\n";
	}
	if (style.hasSpecular()) {
		const double* specular = style.specular();
		mtl_stream << "Ks " << specular[0] << " " << specular[1] << " " << specular[2] << "\n";
	}
	if (style.hasSpecularity()) {
		mtl_stream << "Ns " << style.specularity() << "\n";
	}
	if (style.hasTransparency()) {
		const double transparency = 1.0 - style.transparency();
		if (transparency < 1) {
			mtl_stream << "Tr " << transparency << "\n";
			mtl_stream << "d "  << transparency << "\n";
			mtl_stream << "D "  << transparency << "\n";
		}
	}
}

void WaveFrontOBJSerializer::write(const IfcGeom::TriangulationElement<real_t>* o)
{
    const std::string name = (settings().get(IfcGeom::IteratorSettings::USE_ELEMENT_GUIDS)
        ? o->guid() : (settings().get(IfcGeom::IteratorSettings::USE_ELEMENT_NAMES)
            ? o->name() : o->unique_id()));
    obj_stream << "g " << name << "\n";
	obj_stream << "s 1" << "\n";

    const IfcGeom::Representation::Triangulation<real_t>& mesh = o->geometry();
	
	const int vcount = (int)mesh.verts().size() / 3;
    for ( std::vector<real_t>::const_iterator it = mesh.verts().begin(); it != mesh.verts().end(); ) {
        const real_t x = *(it++);
        const real_t y = *(it++);
        const real_t z = *(it++);
		obj_stream << "v " << x << " " << y << " " << z << "\n";
	}

    for ( std::vector<real_t>::const_iterator it = mesh.normals().begin(); it != mesh.normals().end(); ) {
        const real_t x = *(it++);
        const real_t y = *(it++);
        const real_t z = *(it++);
		obj_stream << "vn " << x << " " << y << " " << z << "\n";
	}

    for (std::vector<real_t>::const_iterator it = mesh.uvs().begin(); it != mesh.uvs().end();) {
        const real_t u = *it++;
        const real_t v = *it++;
        obj_stream << "vt " << u << " " << v << "\n";
    }

	int previous_material_id = -2;
	std::vector<int>::const_iterator material_it = mesh.material_ids().begin();

    const bool has_uvs = !mesh.uvs().empty();
	for ( std::vector<int>::const_iterator it = mesh.faces().begin(); it != mesh.faces().end(); ) {
		
		const int material_id = *(material_it++);
		if (material_id != previous_material_id) {
			const IfcGeom::Material& material = mesh.materials()[material_id];
            std::string material_name = (settings().get(IfcGeom::IteratorSettings::USE_MATERIAL_NAMES)
                ? material.original_name() : material.name());
            IfcUtil::sanitate_material_name(material_name);
			obj_stream << "usemtl " << material_name << "\n";
			if (materials.find(material_name) == materials.end()) {
				writeMaterial(material);
				materials.insert(material_name);
			}
			previous_material_id = material_id;
		}

		const int v1 = *(it++)+vcount_total;
		const int v2 = *(it++)+vcount_total;
		const int v3 = *(it++)+vcount_total;
        obj_stream << "f " << v1 << "/" << (has_uvs ? boost::lexical_cast<std::string>(v1) : "") << "/" << v1 << " "
            << v2 << "/" << (has_uvs ? boost::lexical_cast<std::string>(v2) : "") << "/" << v2 << " "
            << v3 << "/" << (has_uvs ? boost::lexical_cast<std::string>(v3) : "") << "/" << v3 << "\n";

	}

	std::set<int> faces_set (mesh.faces().begin(), mesh.faces().end());
	const std::vector<int>& edges = mesh.edges();

	for ( std::vector<int>::const_iterator it = edges.begin(); it != edges.end(); ) {
		const int i1 = *(it++);
		const int i2 = *(it++);

		if (faces_set.find(i1) != faces_set.end() || faces_set.find(i2) != faces_set.end()) {
			continue;
		}

		const int material_id = *(material_it++);

		if (material_id != previous_material_id) {
			const IfcGeom::Material& material = mesh.materials()[material_id];
            std::string material_name = (settings().get(IfcGeom::IteratorSettings::USE_MATERIAL_NAMES)
                ? material.original_name() : material.name());
            IfcUtil::sanitate_material_name(material_name);
			obj_stream << "usemtl " << material_name << "\n";
			if (materials.find(material_name) == materials.end()) {
				writeMaterial(material);
				materials.insert(material_name);
			}
			previous_material_id = material_id;
		}

		const int v1 = i1 + vcount_total;
		const int v2 = i2 + vcount_total;

		obj_stream << "l " << v1 << " " << v2 << "\n";
	}

	vcount_total += vcount;
}
