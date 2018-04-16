-- Test script file

TestGlobalBS = TestGlobalPSO.GraphicsPipeline.BlendDesc

assert( TestGlobalPSO.Name == "PSO-TestBlendStates");

assert( TestGlobalBS.IndependentBlendEnable == true);
assert( TestGlobalBS.AlphaToCoverageEnable == false);
assert( TestGlobalBS.RenderTargets[1].BlendEnable == true);
assert( TestGlobalBS.RenderTargets[1].SrcBlend       == "BLEND_FACTOR_ZERO");
assert( TestGlobalBS.RenderTargets[1].DestBlend      == "BLEND_FACTOR_ONE");
assert( TestGlobalBS.RenderTargets[1].BlendOp        == "BLEND_OPERATION_ADD");
assert( TestGlobalBS.RenderTargets[1].SrcBlendAlpha  == "BLEND_FACTOR_SRC_ALPHA");
assert( TestGlobalBS.RenderTargets[1].DestBlendAlpha == "BLEND_FACTOR_INV_SRC_ALPHA");
assert( TestGlobalBS.RenderTargets[1].BlendOpAlpha   == "BLEND_OPERATION_SUBTRACT");
assert( TestGlobalBS.RenderTargets[1].RenderTargetWriteMask[1] == "COLOR_MASK_RED");

assert( TestGlobalBS.RenderTargets[2].BlendEnable == true);
assert( TestGlobalBS.RenderTargets[2].SrcBlend       == "BLEND_FACTOR_SRC_ALPHA");
assert( TestGlobalBS.RenderTargets[2].DestBlend      == "BLEND_FACTOR_INV_SRC_ALPHA");
assert( TestGlobalBS.RenderTargets[2].BlendOp        == "BLEND_OPERATION_REV_SUBTRACT");
assert( TestGlobalBS.RenderTargets[2].SrcBlendAlpha  == "BLEND_FACTOR_DEST_ALPHA");
assert( TestGlobalBS.RenderTargets[2].DestBlendAlpha == "BLEND_FACTOR_INV_DEST_ALPHA");
assert( TestGlobalBS.RenderTargets[2].BlendOpAlpha   == "BLEND_OPERATION_MIN");
assert( TestGlobalBS.RenderTargets[2].RenderTargetWriteMask[1] == "COLOR_MASK_GREEN");

assert( TestGlobalBS.RenderTargets[3].BlendEnable == true);
assert( TestGlobalBS.RenderTargets[3].SrcBlend       == "BLEND_FACTOR_DEST_COLOR");
assert( TestGlobalBS.RenderTargets[3].DestBlend      == "BLEND_FACTOR_INV_DEST_COLOR");
assert( TestGlobalBS.RenderTargets[3].BlendOp        == "BLEND_OPERATION_MAX");
assert( TestGlobalBS.RenderTargets[3].SrcBlendAlpha  == "BLEND_FACTOR_SRC_ALPHA_SAT");
assert( TestGlobalBS.RenderTargets[3].DestBlendAlpha == "BLEND_FACTOR_BLEND_FACTOR");
assert( TestGlobalBS.RenderTargets[3].BlendOpAlpha   == "BLEND_OPERATION_ADD");
assert( TestGlobalBS.RenderTargets[3].RenderTargetWriteMask[1] == "COLOR_MASK_BLUE");

assert( TestGlobalBS.RenderTargets[4].BlendEnable == true);
assert( TestGlobalBS.RenderTargets[4].SrcBlend       == "BLEND_FACTOR_INV_BLEND_FACTOR");
assert( TestGlobalBS.RenderTargets[4].DestBlend      == "BLEND_FACTOR_SRC_COLOR");
assert( TestGlobalBS.RenderTargets[4].BlendOp        == "BLEND_OPERATION_MAX");
assert( TestGlobalBS.RenderTargets[4].SrcBlendAlpha  == "BLEND_FACTOR_INV_SRC_ALPHA");
assert( TestGlobalBS.RenderTargets[4].DestBlendAlpha == "BLEND_FACTOR_SRC_ALPHA");
assert( TestGlobalBS.RenderTargets[4].BlendOpAlpha   == "BLEND_OPERATION_ADD");
assert( TestGlobalBS.RenderTargets[4].RenderTargetWriteMask[1] == "COLOR_MASK_ALPHA");

BSDesc = 
{
	IndependentBlendEnable = true,
	AlphaToCoverageEnable = false,
	RenderTargets = 
	{
		{ 
			BlendEnable = true, 
			SrcBlend = "BLEND_FACTOR_ZERO", DestBlend = "BLEND_FACTOR_SRC_COLOR", BlendOp = "BLEND_OPERATION_ADD",
			SrcBlendAlpha = "BLEND_FACTOR_SRC_ALPHA", DestBlendAlpha = "BLEND_FACTOR_INV_SRC_ALPHA", BlendOpAlpha = "BLEND_OPERATION_SUBTRACT",
			RenderTargetWriteMask = {"COLOR_MASK_GREEN", "COLOR_MASK_RED"} 
		},

		[3] = { 
			BlendEnable = true, 
			SrcBlend = "BLEND_FACTOR_INV_DEST_ALPHA", DestBlend = "BLEND_FACTOR_INV_DEST_COLOR", BlendOp = "BLEND_OPERATION_ADD",
			SrcBlendAlpha = "BLEND_FACTOR_BLEND_FACTOR", DestBlendAlpha = "BLEND_FACTOR_INV_SRC_ALPHA", BlendOpAlpha = "BLEND_OPERATION_SUBTRACT",
			RenderTargetWriteMask = {"COLOR_MASK_BLUE", "COLOR_MASK_ALPHA"} 
		},
	}
}


TrivialVS = Shader.Create{
	Source = 
[[void VSMain(out float4 pos : SV_Position)
{
	pos = float4(0.0, 0.0, 0.0, 0.0); 
}]],
	EntryPoint = "VSMain",
	SourceLanguage = "SHADER_SOURCE_LANGUAGE_HLSL",
	Desc = {
		ShaderType = "SHADER_TYPE_VERTEX",
		Name = "TrivialVS (BlendStateTest.lua)"
	}
}

TrivialPS = Shader.Create{
	Source = 
[[void PSMain(out float4 col0 : SV_TARGET, 
              out float4 col1 : SV_TARGET1,
              out float4 col2 : SV_TARGET2,
              out float4 col3 : SV_TARGET3)
{
	col0 = float4(0.0, 0.0, 0.0, 0.0);
    col1 = float4(0.0, 0.0, 0.0, 0.0);
    col2 = float4(0.0, 0.0, 0.0, 0.0);
    col3 = float4(0.0, 0.0, 0.0, 0.0);
}]],
	EntryPoint = "PSMain",
	SourceLanguage = "SHADER_SOURCE_LANGUAGE_HLSL",
	Desc = {
		ShaderType = "SHADER_TYPE_PIXEL",
		Name = "TrivialPS (BlendStateTest.lua)"
	}
}

PSODesc = 
{
	Name = "TestPSO_FromScript",
	GraphicsPipeline = 
	{
		pVS = TrivialVS,
		pPS = TrivialPS,
		BlendDesc = BSDesc,
		RTVFormats = {"TEX_FORMAT_RGBA8_UNORM", "TEX_FORMAT_RGBA8_UNORM", "TEX_FORMAT_RGBA8_UNORM", "TEX_FORMAT_RGBA8_UNORM"},
		DSVFormat = "TEX_FORMAT_D32_FLOAT",
		PrimitiveTopology = "PRIMITIVE_TOPOLOGY_TRIANGLE_LIST",
		SmplDesc = {Count = 1, Quality = 0 }
	}
}
PSO_TestBlendState = PipelineState.Create(PSODesc)
Context.SetBlendFactors()
Context.SetBlendFactors(1)
Context.SetBlendFactors(1,2)
Context.SetBlendFactors(1,2,3)
Context.SetBlendFactors(1,2,3,4)

--TestBS2 = BlendState.Create(BSDesc)

TestBS = PSO_TestBlendState.GraphicsPipeline.BlendDesc

assert( PSO_TestBlendState.Name == "TestPSO_FromScript" )
assert( TestBS.IndependentBlendEnable == true )
assert( TestBS.AlphaToCoverageEnable == false )
RT1 = TestBS.RenderTargets[1]
assert( RT1.BlendEnable == true) 
assert( RT1.SrcBlend == "BLEND_FACTOR_ZERO") 
assert( RT1.DestBlend == "BLEND_FACTOR_SRC_COLOR") 
assert( RT1.BlendOp == "BLEND_OPERATION_ADD")
assert( RT1.SrcBlendAlpha == "BLEND_FACTOR_SRC_ALPHA") 
assert( RT1.DestBlendAlpha == "BLEND_FACTOR_INV_SRC_ALPHA") 
assert( RT1.BlendOpAlpha == "BLEND_OPERATION_SUBTRACT")
assert( RT1.RenderTargetWriteMask[1] == "COLOR_MASK_GREEN" and RT1.RenderTargetWriteMask[2] == "COLOR_MASK_RED" or 
        RT1.RenderTargetWriteMask[2] == "COLOR_MASK_GREEN" and RT1.RenderTargetWriteMask[1] == "COLOR_MASK_RED" )

RT3 = TestBS.RenderTargets[3]
assert(RT3.BlendEnable == true) 
assert(RT3.SrcBlend == "BLEND_FACTOR_INV_DEST_ALPHA") 
assert(RT3.DestBlend == "BLEND_FACTOR_INV_DEST_COLOR") 
assert(RT3.BlendOp == "BLEND_OPERATION_ADD")
assert(RT3.SrcBlendAlpha == "BLEND_FACTOR_BLEND_FACTOR") 
assert(RT3.DestBlendAlpha == "BLEND_FACTOR_INV_SRC_ALPHA") 
assert(RT3.BlendOpAlpha == "BLEND_OPERATION_SUBTRACT")
assert(RT3.RenderTargetWriteMask[1] == "COLOR_MASK_BLUE" and RT3.RenderTargetWriteMask[2] == "COLOR_MASK_ALPHA" or 
       RT3.RenderTargetWriteMask[2] == "COLOR_MASK_BLUE" and RT3.RenderTargetWriteMask[1] == "COLOR_MASK_ALPHA" )

Context.SetPipelineState(PSO_TestBlendState)


BlendFactors =
{
	"BLEND_FACTOR_ZERO",
    "BLEND_FACTOR_ONE",
    "BLEND_FACTOR_SRC_COLOR",
    "BLEND_FACTOR_INV_SRC_COLOR",
    "BLEND_FACTOR_SRC_ALPHA",
    "BLEND_FACTOR_INV_SRC_ALPHA",
    "BLEND_FACTOR_DEST_ALPHA",
    "BLEND_FACTOR_INV_DEST_ALPHA",
    "BLEND_FACTOR_DEST_COLOR",
    "BLEND_FACTOR_INV_DEST_COLOR",
    "BLEND_FACTOR_SRC_ALPHA_SAT",
    "BLEND_FACTOR_BLEND_FACTOR",
    "BLEND_FACTOR_INV_BLEND_FACTOR"
}

AlphaBlendFactors =
{
	"BLEND_FACTOR_ZERO",
    "BLEND_FACTOR_ONE",
    "BLEND_FACTOR_SRC_ALPHA",
    "BLEND_FACTOR_INV_SRC_ALPHA",
    "BLEND_FACTOR_DEST_ALPHA",
    "BLEND_FACTOR_INV_DEST_ALPHA",
    "BLEND_FACTOR_SRC_ALPHA_SAT",
    "BLEND_FACTOR_BLEND_FACTOR",
    "BLEND_FACTOR_INV_BLEND_FACTOR"
}

BlendOps = 
{
    "BLEND_OPERATION_ADD",
    "BLEND_OPERATION_SUBTRACT",
    "BLEND_OPERATION_REV_SUBTRACT",
    "BLEND_OPERATION_MIN",
    "BLEND_OPERATION_MAX"
}

for rt = 1, 4 do
	BSDesc.RenderTargets[rt] = BSDesc.RenderTargets[rt] or {}

    RT = BSDesc.RenderTargets[rt]
	RT.BlendEnable = true
	for i,v in ipairs(BlendFactors) do
		RT.SrcBlend = v
		RT.DestBlend = v
		PSODesc.GraphicsPipeline.BlendDesc = BSDesc
		local TestPSO = PipelineState.Create(PSODesc)
        assert(TestPSO:IsCompatibleWith(PSO_TestBlendState) == true)
        assert(PSO_TestBlendState:IsCompatibleWith(TestPSO) == true)
		assert(TestPSO.GraphicsPipeline.BlendDesc.RenderTargets[rt].SrcBlend == v)
		assert(TestPSO.GraphicsPipeline.BlendDesc.RenderTargets[rt].DestBlend == v)
		Context.SetPipelineState(TestPSO)
	end

	for i,v in ipairs(AlphaBlendFactors) do
		RT.SrcBlendAlpha = v
		RT.DestBlendAlpha = v
		PSODesc.GraphicsPipeline.BlendDesc = BSDesc
		local TestPSO = PipelineState.Create(PSODesc)
        assert(TestPSO:IsCompatibleWith(PSO_TestBlendState) == true)
        assert(PSO_TestBlendState:IsCompatibleWith(TestPSO) == true)
		assert(TestPSO.GraphicsPipeline.BlendDesc.RenderTargets[rt].SrcBlendAlpha == v)
		assert(TestPSO.GraphicsPipeline.BlendDesc.RenderTargets[rt].DestBlendAlpha == v)
		Context.SetPipelineState(TestPSO)
	end

    RT.SrcBlend  = "BLEND_FACTOR_SRC_COLOR"
    RT.DestBlend = "BLEND_FACTOR_INV_SRC_COLOR"
    RT.SrcBlendAlpha  = "BLEND_FACTOR_SRC_ALPHA"
    RT.DestBlendAlpha = "BLEND_FACTOR_INV_SRC_ALPHA"
	for i,v in ipairs(BlendOps) do
		RT.BlendOp = v
		RT.BlendOpAlpha = v
		PSODesc.GraphicsPipeline.BlendDesc = BSDesc
		local TestPSO = PipelineState.Create(PSODesc)
        assert(TestPSO:IsCompatibleWith(PSO_TestBlendState) == true)
        assert(PSO_TestBlendState:IsCompatibleWith(TestPSO) == true)
		assert(TestPSO.GraphicsPipeline.BlendDesc.RenderTargets[rt].BlendOp == v)
		assert(TestPSO.GraphicsPipeline.BlendDesc.RenderTargets[rt].BlendOpAlpha == v)
		Context.SetPipelineState(TestPSO)
	end
end

function TestPSOArg(PSOArg)
	assert(PSOArg.Name            == "PSO-TestBlendStates" )
	local BSDesc = PSOArg.GraphicsPipeline.BlendDesc
	assert(BSDesc.AlphaToCoverageEnable  == false) 
    assert(BSDesc.IndependentBlendEnable == false)

	for rt = 1, 8 do
		local RT = BSDesc.RenderTargets[rt]
		assert( RT.BlendEnable    == false )
		assert( RT.SrcBlend       == "BLEND_FACTOR_ONE" )
		assert( RT.SrcBlend       == "BLEND_FACTOR_ONE" )
		assert( RT.DestBlend      == "BLEND_FACTOR_ZERO" )
		assert( RT.BlendOp        == "BLEND_OPERATION_ADD" )
		assert( RT.SrcBlendAlpha  == "BLEND_FACTOR_ONE" )
		assert( RT.DestBlendAlpha == "BLEND_FACTOR_ZERO" )
		assert( RT.BlendOpAlpha   == "BLEND_OPERATION_ADD" )
	end
end
