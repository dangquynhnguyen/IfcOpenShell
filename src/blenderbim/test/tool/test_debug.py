# BlenderBIM Add-on - OpenBIM Blender Add-on
# Copyright (C) 2022 Dion Moult <dion@thinkmoult.com>
#
# This file is part of BlenderBIM Add-on.
#
# BlenderBIM Add-on is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# BlenderBIM Add-on is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with BlenderBIM Add-on.  If not, see <http://www.gnu.org/licenses/>.

import os
import bpy
import ifcopenshell
import blenderbim.core.tool
import blenderbim.tool as tool
from test.bim.bootstrap import NewFile
from blenderbim.tool.debug import Debug as subject
from blenderbim.bim.ifc import IfcStore


class TestImplementsTool(NewFile):
    def test_run(self):
        assert isinstance(subject(), blenderbim.core.tool.Debug)


class TestAddSchemaIdentifier(NewFile):
    def test_run(self):
        subject.add_schema_identifier(TestLoadExpress().test_run())
        assert IfcStore.schema_identifiers[-1] == "IFCROGUE"


class TestLoadExpress(NewFile):
    def test_run(self):
        cwd = os.path.dirname(os.path.realpath(__file__))
        schema = subject.load_express(os.path.join(cwd, "..", "files", "test.exp"))
        assert schema.schema_name == "IFCROGUE"
        return schema
