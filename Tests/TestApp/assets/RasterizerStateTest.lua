-- Test script file

TestGlobalRSDesc = TestGlobalPSO.GraphicsPipeline.RasterizerDesc

assert( TestGlobalPSO.Name == "PSO-TestRS")
assert( TestGlobalRSDesc.FillMode == "FILL_MODE_WIREFRAME")
assert( TestGlobalRSDesc.CullMode == "CULL_MODE_FRONT")
assert( TestGlobalRSDesc.FrontCounterClockwise == true)
assert( TestGlobalRSDesc.DepthBias == 64)
assert( TestGlobalRSDesc.DepthBiasClamp == 98.0)
assert( TestGlobalRSDesc.SlopeScaledDepthBias == 12.5)
assert( TestGlobalRSDesc.DepthClipEnable == false)
assert( TestGlobalRSDesc.ScissorEnable == false)
assert( TestGlobalRSDesc.AntialiasedLineEnable == true)


RSDesc = 
{
	FillMode = "FILL_MODE_WIREFRAME",
	CullMode = "CULL_MODE_BACK",
	FrontCounterClockwise = true,
	DepthBias = 32,
	DepthBiasClamp = 63.0,
	SlopeScaledDepthBias = 31.25,
	DepthClipEnable = true,
	ScissorEnable = false,
	AntialiasedLineEnable = true,
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
		Name = "TrivialVS (RasterizerStateTest.lua)"
	}
}

TrivialPS = Shader.Create{
	Source = 
[[void PSMain(out float4 col : SV_TARGET)
{
	col = float4(0.0, 0.0, 0.0, 0.0);
}]],
	EntryPoint = "PSMain",
	SourceLanguage = "SHADER_SOURCE_LANGUAGE_HLSL",
	Desc = {
		ShaderType = "SHADER_TYPE_PIXEL",
		Name = "TrivialPS (RasterizerStateTest.lua)"
	}
}

PSODesc = 
{
	GraphicsPipeline = 
	{
		pVS = TrivialVS,
		pPS = TrivialPS,
		RasterizerDesc = RSDesc,
		RTVFormats = "TEX_FORMAT_RGBA8_UNORM",
		DSVFormat = "TEX_FORMAT_D32_FLOAT",
		PrimitiveTopologyType = "PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE"
	}
}
TestRS_PSO = PipelineState.Create(PSODesc)
--TestRS2 = RasterizerState.Create(RSDesc)
TestRS = TestRS_PSO.GraphicsPipeline.RasterizerDesc

assert( TestRS.FillMode == "FILL_MODE_WIREFRAME")
assert( TestRS.CullMode == "CULL_MODE_BACK")
assert( TestRS.FrontCounterClockwise == true)
assert( TestRS.DepthBias == 32)
assert( TestRS.DepthBiasClamp == 63.0)
assert( TestRS.SlopeScaledDepthBias == 31.25)
assert( TestRS.DepthClipEnable == true)
assert( TestRS.ScissorEnable == false)
assert( TestRS.AntialiasedLineEnable == true)

Context.SetPipelineState(TestRS_PSO)
--Context.SetRasterizerState(TestRS)


FillModes =
{
	"FILL_MODE_WIREFRAME",
	"FILL_MODE_SOLID"
}

CullModes = 
{
    "CULL_MODE_BACK",
    "CULL_MODE_FRONT",
    "CULL_MODE_NONE"
}

for i,v in ipairs(FillModes) do
	PSODesc.GraphicsPipeline.RasterizerDesc.FillMode = v
	local TestPSO = PipelineState.Create(PSODesc)
	assert(TestPSO.GraphicsPipeline.RasterizerDesc.FillMode == v)
	Context.SetPipelineState(TestPSO)
end

for i,v in ipairs(CullModes) do
	PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = v
	local TestPSO = PipelineState.Create(PSODesc)
	assert(TestPSO.GraphicsPipeline.RasterizerDesc.CullMode == v)
	Context.SetPipelineState(TestPSO)
end


function TestRSDescFunc(PSO)
	
	local RSDesc = PSO.GraphicsPipeline.RasterizerDesc
    assert( RSDesc.FillMode == "FILL_MODE_SOLID")
	assert( RSDesc.CullMode == "CULL_MODE_BACK")
	assert( RSDesc.FrontCounterClockwise == false)
	assert( RSDesc.DepthBias == 0)
	assert( RSDesc.DepthBiasClamp == 0.0)
	assert( RSDesc.SlopeScaledDepthBias == 0.0)
	assert( RSDesc.DepthClipEnable == true)
	assert( RSDesc.ScissorEnable == false)
	assert( RSDesc.AntialiasedLineEnable == false)
end
