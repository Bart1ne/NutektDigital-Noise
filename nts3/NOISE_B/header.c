#include "unit_genericfx.h"
// Development-only IDs: replace dev_id with registered KORG developer ID for distribution.
const __unit_header genericfx_unit_header_t unit_header = {
 .common = {
  .header_size = sizeof(genericfx_unit_header_t),
  .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
  .api = UNIT_API_VERSION,
  .dev_id = 0x4E4C4142U,
  .unit_id = 0x4e4f4231U,
  .version = 0x00010200U,
  .name = "NOISE_B",
  .num_params = 8,
  .params = {
    {0,5,0,0,k_unit_param_type_strings,0,0,0,{"TYPE"}},
    {0,1023,0,512,k_unit_param_type_none,0,0,0,{"TONE"}},
    {0,1023,0,370,k_unit_param_type_none,0,0,0,{"DECAY"}},
    {0,1023,0,300,k_unit_param_type_none,0,0,0,{"LEVEL"}},
    {0,1023,0,0,k_unit_param_type_none,0,0,0,{"MOTION"}},
    {0,1023,0,300,k_unit_param_type_none,0,0,0,{"RATE"}},
    {0,1023,0,512,k_unit_param_type_none,0,0,0,{"DENSITY"}},
    {0,1,0,1,k_unit_param_type_strings,0,0,0,{"MODE"}}
  }
 },
 .default_mappings = {
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,5,0},
    {k_genericfx_param_assign_x,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,512},
    {k_genericfx_param_assign_y,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,370},
    {k_genericfx_param_assign_depth,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,300},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,0},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,300},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,512},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1,1}
 }
};
