/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <gtest/gtest.h>

#include <mcs/mediamanagerfactory.h>
#include <mcs/mirsourcemediamanager.h>
#include <mcs/testsourcemediamanager.h>

struct MediaManagerFactoryFixture : public ::testing::Test {
    template <typename T>
    void CheckSourceCreation(const std::string &type_name) {
        setenv("MIRACAST_SOURCE_TYPE", type_name.c_str(), 1);
        EXPECT_TRUE((std::dynamic_pointer_cast<T>(mcs::MediaManagerFactory::CreateSource("")) ? true : false));
    }
};

TEST_F(MediaManagerFactoryFixture, InvalidType) {
    CheckSourceCreation<mcs::NullSourceMediaManager>("foobar");
}

TEST_F(MediaManagerFactoryFixture, DefaultType) {
    CheckSourceCreation<mcs::MirSourceMediaManager>("");
}

TEST_F(MediaManagerFactoryFixture, MirTypeCreation) {
    CheckSourceCreation<mcs::MirSourceMediaManager>("mir");
    CheckSourceCreation<mcs::NullSourceMediaManager>("mir123");
    CheckSourceCreation<mcs::NullSourceMediaManager>("123mir123");
    CheckSourceCreation<mcs::NullSourceMediaManager>("123mir");
}

TEST_F(MediaManagerFactoryFixture, TestTypeCreation) {
    CheckSourceCreation<mcs::TestSourceMediaManager>("test");
    CheckSourceCreation<mcs::NullSourceMediaManager>("test123");
    CheckSourceCreation<mcs::NullSourceMediaManager>("123test123");
    CheckSourceCreation<mcs::NullSourceMediaManager>("123test");
}
