-- root workspace, all sub-project should be included
workspace "learning_vk"
    location("build/" .. _ACTION)
    configurations { "Debug", "Development", "Release" }

os.chdir(_MAIN_SCRIPT_DIR)

local r = require("premake/rythe")

r.configure()