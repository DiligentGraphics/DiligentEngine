-- Test script file

TestGlobalDSState = TestGlobalPSO.GraphicsPipeline.DepthStencilDesc

assert( TestGlobalDSState.DepthEnable == true)
assert( TestGlobalDSState.DepthWriteEnable == true)
assert( TestGlobalDSState.DepthFunc == "COMPARISON_FUNC_NEVER")
assert( TestGlobalDSState.StencilEnable == true)
assert( TestGlobalDSState.StencilReadMask == 0xFA)
assert( TestGlobalDSState.StencilWriteMask == 0xFF)

assert( TestGlobalDSState.FrontFace.StencilFailOp == "STENCIL_OP_DECR_SAT")
assert( TestGlobalDSState.FrontFace.StencilDepthFailOp == "STENCIL_OP_DECR_WRAP")
assert( TestGlobalDSState.FrontFace.StencilPassOp == "STENCIL_OP_INCR_WRAP")
assert( TestGlobalDSState.FrontFace.StencilFunc == "COMPARISON_FUNC_NOT_EQUAL")

assert( TestGlobalDSState.BackFace.StencilFailOp == "STENCIL_OP_INVERT")
assert( TestGlobalDSState.BackFace.StencilDepthFailOp == "STENCIL_OP_REPLACE" )
assert( TestGlobalDSState.BackFace.StencilPassOp == "STENCIL_OP_INCR_SAT")
assert( TestGlobalDSState.BackFace.StencilFunc == "COMPARISON_FUNC_EQUAL")

DSSDesc = 
{
    DepthEnable = true,
    DepthWriteEnable = true,
    DepthFunc = "COMPARISON_FUNC_LESS",
    StencilEnable = true,
    StencilReadMask = 0xF8,
    StencilWriteMask = 0xF1,
    FrontFace = {StencilFailOp = "STENCIL_OP_KEEP",  StencilDepthFailOp = "STENCIL_OP_ZERO", StencilPassOp = "STENCIL_OP_REPLACE", StencilFunc = "COMPARISON_FUNC_EQUAL" },
    BackFace = {StencilFailOp = "STENCIL_OP_INCR_SAT",  StencilDepthFailOp = "STENCIL_OP_DECR_SAT", StencilPassOp = "STENCIL_OP_INVERT", StencilFunc = "COMPARISON_FUNC_NOT_EQUAL"}
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
		Name = "TrivialVS (DepthStencilStateTest.lua)"
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
		Name = "TrivialPS (DepthStencilStateTest.lua)"
	}
}

PSODesc = 
{
	GraphicsPipeline = 
	{
		pVS = TrivialVS,
		pPS = TrivialPS,
		DepthStencilDesc = DSSDesc,
		RTVFormats = {"TEX_FORMAT_RGBA8_UNORM"},
		DSVFormat = "TEX_FORMAT_D32_FLOAT",
		PrimitiveTopologyType = "PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE"
	}
}


TestPSO = PipelineState.Create(PSODesc)
--TestDSS2 = DepthStencilState.Create(DSSDesc)

TestDSS = TestPSO.GraphicsPipeline.DepthStencilDesc

assert( TestDSS.DepthEnable == true )
assert( TestDSS.DepthWriteEnable == true )
assert( TestDSS.DepthFunc == "COMPARISON_FUNC_LESS" )
assert( TestDSS.StencilEnable == true )
assert( TestDSS.StencilReadMask == 0xF8 )
assert( TestDSS.StencilWriteMask == 0xF1 )
assert( TestDSS.FrontFace.StencilFailOp == "STENCIL_OP_KEEP" )
assert( TestDSS.FrontFace.StencilDepthFailOp == "STENCIL_OP_ZERO") 
assert( TestDSS.FrontFace.StencilPassOp == "STENCIL_OP_REPLACE")
assert( TestDSS.FrontFace.StencilFunc == "COMPARISON_FUNC_EQUAL")
assert( TestDSS.BackFace.StencilFailOp == "STENCIL_OP_INCR_SAT")
assert( TestDSS.BackFace.StencilDepthFailOp == "STENCIL_OP_DECR_SAT")
assert( TestDSS.BackFace.StencilPassOp == "STENCIL_OP_INVERT")
assert( TestDSS.BackFace.StencilFunc == "COMPARISON_FUNC_NOT_EQUAL")

Context.SetPipelineState(TestPSO)
--Context.SetDepthStencilState(TestDSS,2)

ComparisonFuncs =
{
	"COMPARISON_FUNC_NEVER",
	"COMPARISON_FUNC_LESS",
	"COMPARISON_FUNC_EQUAL",
	"COMPARISON_FUNC_LESS_EQUAL",
	"COMPARISON_FUNC_GREATER",
	"COMPARISON_FUNC_NOT_EQUAL",
	"COMPARISON_FUNC_GREATER_EQUAL",
	"COMPARISON_FUNC_ALWAYS"
}

StencilOps = 
{
    "STENCIL_OP_KEEP",
    "STENCIL_OP_ZERO",
    "STENCIL_OP_REPLACE",
    "STENCIL_OP_INCR_SAT",
    "STENCIL_OP_DECR_SAT",
    "STENCIL_OP_INVERT",
    "STENCIL_OP_INCR_WRAP",
    "STENCIL_OP_DECR_WRAP"
}

for i,v in ipairs(ComparisonFuncs) do
	PSODesc.GraphicsPipeline.DepthStencilDesc.DepthFunc = v
	local TestPSO2 = PipelineState.Create(PSODesc)
    assert(TestPSO:IsCompatibleWith(TestPSO2))
    assert(TestPSO2:IsCompatibleWith(TestPSO))
	assert(TestPSO2.GraphicsPipeline.DepthStencilDesc.DepthFunc == v)
	Context.SetPipelineState(TestPSO2)
end

for i,v in ipairs(StencilOps) do
	PSODesc.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp = v
	PSODesc.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp = v
	PSODesc.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp = v
	local TestPSO2 = PipelineState.Create(PSODesc)
    assert(TestPSO:IsCompatibleWith(TestPSO2))
    assert(TestPSO2:IsCompatibleWith(TestPSO))
	assert( TestPSO2.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp == v )
	assert( TestPSO2.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp == v )
	assert( TestPSO2.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp == v )
	Context.SetPipelineState(TestPSO2)
end

for i,v in ipairs(StencilOps) do
	PSODesc.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp = v
	PSODesc.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp = v
	PSODesc.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp = v
	local TestPSO2 = PipelineState.Create(PSODesc)
    assert(TestPSO:IsCompatibleWith(TestPSO2))
    assert(TestPSO2:IsCompatibleWith(TestPSO))
	--local TestDSS2 = DepthStencilState.Create(DSSDesc)
	assert( TestPSO2.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp == v )
	assert( TestPSO2.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp == v )
	assert( TestPSO2.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp == v )
	Context.SetPipelineState(TestPSO2)
end


for i,v in ipairs(ComparisonFuncs) do
	PSODesc.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc = v
	PSODesc.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc = v
	local TestPSO2 = PipelineState.Create(PSODesc)
    assert(TestPSO:IsCompatibleWith(TestPSO2))
    assert(TestPSO2:IsCompatibleWith(TestPSO))
	assert( TestPSO2.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc == v )
	assert( TestPSO2.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc == v )
	Context.SetPipelineState(TestPSO2)
end

function TestDSSDesc(PSOArg)
	DSSArg = PSOArg.GraphicsPipeline.DepthStencilDesc

    assert(DSSArg.DepthEnable     == true )
    assert(DSSArg.DepthWriteEnable== true )
    assert(DSSArg.DepthFunc       == "COMPARISON_FUNC_LESS" )
    assert(DSSArg.StencilEnable   == false )
    assert(DSSArg.StencilReadMask == 0xFF )
    assert(DSSArg.StencilWriteMask== 0xFF )
	
    assert(DSSArg.FrontFace.StencilFailOp      == "STENCIL_OP_KEEP" )
    assert(DSSArg.FrontFace.StencilDepthFailOp == "STENCIL_OP_KEEP" )
	assert(DSSArg.FrontFace.StencilPassOp      == "STENCIL_OP_KEEP" )
    assert(DSSArg.FrontFace.StencilFunc        == "COMPARISON_FUNC_ALWAYS" )

    assert(DSSArg.BackFace.StencilFailOp      == "STENCIL_OP_KEEP" )
    assert(DSSArg.BackFace.StencilDepthFailOp == "STENCIL_OP_KEEP" )
	assert(DSSArg.BackFace.StencilPassOp      == "STENCIL_OP_KEEP" )
    assert(DSSArg.BackFace.StencilFunc        == "COMPARISON_FUNC_ALWAYS" )
end
