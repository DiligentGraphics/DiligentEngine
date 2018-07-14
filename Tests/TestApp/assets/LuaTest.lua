-- Test script file

assert( TestGlobalBool == true )
assert( TestGlobalInt == 19 )
assert( TestGlobalFloat == 139.25 )
assert( TestGlobalString == "Test Global String" )

assert(TestGlobalBuffer.uiSizeInBytes == 256)
assert(TestGlobalBuffer.Name == "TestGlobalBuff")
assert(TestGlobalBuffer.Usage == "USAGE_DYNAMIC")
assert(TestGlobalBuffer.BindFlags[1] == "BIND_UNIFORM_BUFFER")
assert(TestGlobalBuffer.CPUAccessFlags[1] == "CPU_ACCESS_WRITE")


assert( TestGlobalSampler.Name == "Test Sampler" )
assert( TestGlobalSampler.MinFilter == "FILTER_TYPE_COMPARISON_POINT" )
assert( TestGlobalSampler.MagFilter == "FILTER_TYPE_COMPARISON_LINEAR" )
assert( TestGlobalSampler.MipFilter == "FILTER_TYPE_COMPARISON_LINEAR" )
assert( TestGlobalSampler.AddressU == "TEXTURE_ADDRESS_WRAP" )
assert( TestGlobalSampler.AddressV == "TEXTURE_ADDRESS_MIRROR" )
assert( TestGlobalSampler.AddressW == "TEXTURE_ADDRESS_CLAMP" )
assert( TestGlobalSampler.MipLODBias == 4 )
assert( TestGlobalSampler.MinLOD == 1.5 )
assert( TestGlobalSampler.MaxLOD == 4 )
assert( TestGlobalSampler.MaxAnisotropy == 2 )
assert( TestGlobalSampler.ComparisonFunc == "COMPARISON_FUNC_LESS" )
assert( TestGlobalSampler.BorderColor.r == 0.0 )
assert( TestGlobalSampler.BorderColor.g == 0.0 )
assert( TestGlobalSampler.BorderColor.b == 0.0 )
assert( TestGlobalSampler.BorderColor.a == 1.0 )

assert( TestGlobalBuffer2UAV.ViewType == "BUFFER_VIEW_UNORDERED_ACCESS" )
assert( TestGlobalBuffer2UAV.ByteOffset == 0 )
assert( TestGlobalBuffer2UAV.ByteWidth == 64 )

function CheckTextureAttribs(Texture)
	assert( Texture.Type == "RESOURCE_DIM_TEX_2D" )
	assert( Texture.Name == "Test Global Texture 2D" )
	assert( Texture.Width == 1024 )
	assert( Texture.Height == 512 )
	assert( Texture.MipLevels == 1 )
	assert( Texture.Format == "TEX_FORMAT_RGBA8_UNORM" )
	assert( Texture.Usage == "USAGE_DYNAMIC" )
	assert( Texture.BindFlags[1] == "BIND_SHADER_RESOURCE" )
	assert( Texture.CPUAccessFlags[1] == "CPU_ACCESS_WRITE" )
end
CheckTextureAttribs( TestGlobalTexture )

TestSampler = Sampler.Create{
    Name = "Test Sampler",
    MinFilter = "FILTER_TYPE_POINT", 
    MagFilter = "FILTER_TYPE_LINEAR", 
    MipFilter = "FILTER_TYPE_POINT", 
    AddressU = "TEXTURE_ADDRESS_WRAP", 
    AddressV = "TEXTURE_ADDRESS_MIRROR", 
    AddressW = "TEXTURE_ADDRESS_CLAMP",
    MipLODBias = 2,
    MinLOD = 0.5,
    MaxLOD = 10,
    MaxAnisotropy = 6,
    ComparisonFunc = "COMPARISON_FUNC_GREATER_EQUAL",
    BorderColor = {r = 0.0, g = 0.0, b=0.0, a=1.0},
}
assert( TestSampler.Name == "Test Sampler" )
assert( TestSampler.MinFilter == "FILTER_TYPE_POINT" ) 
assert( TestSampler.MagFilter == "FILTER_TYPE_LINEAR" ) 
assert( TestSampler.MipFilter == "FILTER_TYPE_POINT" )
assert( TestSampler.AddressU == "TEXTURE_ADDRESS_WRAP" ) 
assert( TestSampler.AddressV == "TEXTURE_ADDRESS_MIRROR" ) 
assert( TestSampler.AddressW == "TEXTURE_ADDRESS_CLAMP" )
assert( TestSampler.MipLODBias == 2 )
assert( TestSampler.MinLOD == 0.5 )
assert( TestSampler.MaxLOD == 10 )
assert( TestSampler.MaxAnisotropy == 6 )
assert( TestSampler.ComparisonFunc == "COMPARISON_FUNC_GREATER_EQUAL" )
assert( TestSampler.BorderColor.r == 0.0 )
assert( TestSampler.BorderColor.g == 0.0 )
assert( TestSampler.BorderColor.b == 0.0 )
assert( TestSampler.BorderColor.a == 1.0 )



TestSampler2 = Sampler.Create{
    Name = TestSampler.Name,
    MinFilter = TestSampler.MinFilter, 
    MagFilter = TestSampler.MagFilter, 
    MipFilter = TestSampler.MipFilter, 
    AddressU = TestSampler.AddressU, 
    AddressV = TestSampler.AddressV, 
    AddressW = TestSampler.AddressW,
    MipLODBias = TestSampler.MipLODBias,
    MinLOD = TestSampler.MinLOD,
    MaxLOD = TestSampler.MaxLOD,
    MaxAnisotropy = TestSampler.MaxAnisotropy,
    ComparisonFunc = TestSampler.ComparisonFunc,
    BorderColor = TestSampler.BorderColor,
}
assert( TestSampler2.Name == "Test Sampler" )
assert( TestSampler2.MinFilter == "FILTER_TYPE_POINT" ) 
assert( TestSampler2.MagFilter == "FILTER_TYPE_LINEAR" ) 
assert( TestSampler2.MipFilter == "FILTER_TYPE_POINT" )
assert( TestSampler2.AddressU == "TEXTURE_ADDRESS_WRAP" ) 
assert( TestSampler2.AddressV == "TEXTURE_ADDRESS_MIRROR" ) 
assert( TestSampler2.AddressW == "TEXTURE_ADDRESS_CLAMP" )
assert( TestSampler2.MipLODBias == 2 )
assert( TestSampler2.MinLOD == 0.5 )
assert( TestSampler2.MaxLOD == 10 )
assert( TestSampler2.MaxAnisotropy == 6 )
assert( TestSampler2.ComparisonFunc == "COMPARISON_FUNC_GREATER_EQUAL" )
assert( TestSampler2.BorderColor.r == 0.0 )
assert( TestSampler2.BorderColor.g == 0.0 )
assert( TestSampler2.BorderColor.b == 0.0 )
assert( TestSampler2.BorderColor.a == 1.0 )



TestSampler3 = Sampler.Create{
    BorderColor = 
		{r = TestSampler2.BorderColor.r, 
	     g = TestSampler2.BorderColor.g,
		 b = TestSampler2.BorderColor.b,
		 a = TestSampler2.BorderColor.a
		 }
}
assert( TestSampler3.BorderColor.r == 0.0 )
assert( TestSampler3.BorderColor.g == 0.0 )
assert( TestSampler3.BorderColor.b == 0.0 )
assert( TestSampler3.BorderColor.a == 1.0 )

-- TestSampler3.MaxLOD = 0
TestSampler3.BorderColor.a = 1
assert( TestSampler3.BorderColor.a ==1.0 )

function Func1()
	local TestSampler = Sampler.Create{
    MinFilter = "FILTER_TYPE_POINT"
	}
end

function Func2()
	Func1()
end

Func2()


TestSampler4 = Sampler.Create{}


ErrorTestSampler = Sampler.Create
{
	SomeUndefinedMember = Undefined,
	MinFilter = Undefined2
}

function GetShaderPath( ShaderName, ShaderExt )
	
	local ProcessedShaderPath = ""
	if Constants.DeviceType == "D3D11" or Constants.DeviceType == "D3D12" then
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "DX." .. ShaderExt
	else
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "GL." .. ShaderExt
	end

	return ProcessedShaderPath
end

TestVS = Shader.Create{
	FilePath =  GetShaderPath("minimalInst", "vsh"),
	Desc = {
		ShaderType = "SHADER_TYPE_VERTEX",
		Name = "TestVS"
	}
}
assert(TestVS.Desc.ShaderType == "SHADER_TYPE_VERTEX")
assert(TestVS.Desc.Name == "TestVS")

TestPS = Shader.Create{
	FilePath =  GetShaderPath("minimal", "psh"),
	Desc = {
		ShaderType = "SHADER_TYPE_PIXEL",
		Name = "TestPS",
	}
}
assert(TestPS.Desc.ShaderType == "SHADER_TYPE_PIXEL")
assert(TestPS.Desc.Name == "TestPS")

TestPS2 = Shader.Create{
	FilePath =  GetShaderPath("minimal", "psh"),
	Desc = {
		ShaderType = TestPS.Desc.ShaderType,
		Name = TestPS.Desc.Name .. "2"
	}
}
assert(TestPS2.Desc.ShaderType == "SHADER_TYPE_PIXEL")
assert(TestPS2.Desc.Name == "TestPS2")

UniformBuffPS = Shader.Create{
	FilePath =  GetShaderPath("UniformBuffer", "psh"),
	Desc = {
		ShaderType = "SHADER_TYPE_PIXEL",
		Name = "Unifrom buffer PS"
	}
}

svTestBlock = UniformBuffPS:GetShaderVariable("cbTestBlock")
svTestBlock2 = ShaderVariable.Create(UniformBuffPS, "cbTestBlock2")
svTestBlock2:Set(nil)
--Test = svTestBlock2["Error"]
--svTestBlock2["Error"] = 0

function TestShaderVariable(inVar)
	inVar:Set(nil)
end

TestVertexLayoutPSO = PipelineState.Create
{
	Name = "Test Vertex Layout PSO",
	GraphicsPipeline = 
	{
		InputLayout = 
		{
			{ InputIndex = 0, BufferSlot = 0, NumComponents = 3, ValueType = "VT_FLOAT32", IsNormalized = false},
			{ InputIndex = 1, BufferSlot = 1, NumComponents = 4, ValueType = "VT_UINT8",   IsNormalized = true},
			{ InputIndex = 2, BufferSlot = 2, NumComponents = 2, ValueType = "VT_FLOAT32", IsNormalized = false, Frequency = "FREQUENCY_PER_INSTANCE", InstanceDataStepRate = 1},
		},
		pVS = TestVS,
		pPS = TestPS,
		RTVFormats = "TEX_FORMAT_RGBA8_UNORM_SRGB",
		DSVFormat = "TEX_FORMAT_D32_FLOAT",
	}
}


function CheckVertexDescription(VD)
	assert(VD[1].InputIndex == 0)
	assert(VD[1].BufferSlot == 0)
	assert(VD[1].NumComponents == 3)
	assert(VD[1].ValueType == "VT_FLOAT32")
	assert(VD[1].IsNormalized == false)
	assert(VD[2].InputIndex == 1)
	assert(VD[2].BufferSlot == 1)
	assert(VD[2].NumComponents == 4)
	assert(VD[2].ValueType == "VT_UINT8")
	assert(VD[2].IsNormalized == true)
	assert(VD[3].InputIndex == 2)
	assert(VD[3].BufferSlot == 2)
	assert(VD[3].NumComponents == 2)
	assert(VD[3].ValueType == "VT_FLOAT32")
	assert(VD[3].IsNormalized == false)
	assert(VD[3].Frequency == "FREQUENCY_PER_INSTANCE")
	assert(VD[3].InstanceDataStepRate == 1)
end

CheckVertexDescription(TestVertexLayoutPSO.GraphicsPipeline.InputLayout)

TestVertexLayoutPSO2 = PipelineState.Create
{
	Name = "Test Vertex Layout PSO",
	GraphicsPipeline = 
	{
		InputLayout = TestVertexLayoutPSO.GraphicsPipeline.InputLayout,
		pVS = TestVS,
		pPS = TestPS,
		RTVFormats = "TEX_FORMAT_RGBA8_UNORM_SRGB",
		DSVFormat = "TEX_FORMAT_D32_FLOAT",
	}
}

CheckVertexDescription(TestVertexLayoutPSO2.GraphicsPipeline.InputLayout)


TestVertexLayoutPSO3 = PipelineState.Create
{
	Name = "Test Vertex Layout PSO",
	GraphicsPipeline = 
	{
		InputLayout = 
		{
			TestVertexLayoutPSO.GraphicsPipeline.InputLayout[1],
			{
				InputIndex = TestVertexLayoutPSO.GraphicsPipeline.InputLayout[2].InputIndex,
				BufferSlot = TestVertexLayoutPSO.GraphicsPipeline.InputLayout[2].BufferSlot,
				NumComponents = TestVertexLayoutPSO.GraphicsPipeline.InputLayout[2].NumComponents, 
				ValueType = TestVertexLayoutPSO.GraphicsPipeline.InputLayout[2].ValueType,
				IsNormalized = TestVertexLayoutPSO.GraphicsPipeline.InputLayout[2].IsNormalized
			},
			{
				InputIndex = TestVertexLayoutPSO2.GraphicsPipeline.InputLayout[3].InputIndex,
				BufferSlot = TestVertexLayoutPSO2.GraphicsPipeline.InputLayout[3].BufferSlot,
				NumComponents = TestVertexLayoutPSO2.GraphicsPipeline.InputLayout[3].NumComponents, 
				ValueType = TestVertexLayoutPSO2.GraphicsPipeline.InputLayout[3].ValueType,
				IsNormalized = TestVertexLayoutPSO2.GraphicsPipeline.InputLayout[3].IsNormalized,
				Frequency = TestVertexLayoutPSO2.GraphicsPipeline.InputLayout[3].Frequency,
				InstanceDataStepRate = TestVertexLayoutPSO2.GraphicsPipeline.InputLayout[3].InstanceDataStepRate,
			}			
		},
		pVS = TestVS,
		pPS = TestPS,
		RTVFormats = "TEX_FORMAT_RGBA8_UNORM_SRGB",
		DSVFormat = "TEX_FORMAT_D32_FLOAT",
	}
}

assert(TestVertexLayoutPSO:IsCompatibleWith(TestVertexLayoutPSO))
assert(TestVertexLayoutPSO:IsCompatibleWith(TestVertexLayoutPSO2))
assert(TestVertexLayoutPSO:IsCompatibleWith(TestVertexLayoutPSO3))
assert(TestVertexLayoutPSO2:IsCompatibleWith(TestVertexLayoutPSO))
assert(TestVertexLayoutPSO2:IsCompatibleWith(TestVertexLayoutPSO2))
assert(TestVertexLayoutPSO2:IsCompatibleWith(TestVertexLayoutPSO3))
assert(TestVertexLayoutPSO3:IsCompatibleWith(TestVertexLayoutPSO))
assert(TestVertexLayoutPSO3:IsCompatibleWith(TestVertexLayoutPSO2))
assert(TestVertexLayoutPSO3:IsCompatibleWith(TestVertexLayoutPSO3))

CheckVertexDescription(TestVertexLayoutPSO3.GraphicsPipeline.InputLayout)



function TestRenderScript ()
	Context.SetShaders(TestVS, TestPS)
end

function TestRenderScriptParams (Bool1, MagicNumber1, MagicNumber2, Bool2, MagicString)
	assert(Bool1 == true)
	assert(MagicNumber1 == 123)
	assert(MagicNumber2 == 345.5)
	assert(Bool2 == false)
	assert(MagicString == "Magic String")
end


function TestSamplerArg(Sampler)
	assert( Sampler.Name == "Test Sampler" )
	assert( Sampler.MinFilter == "FILTER_TYPE_POINT" ) 
	assert( Sampler.MagFilter == "FILTER_TYPE_LINEAR" ) 
	assert( Sampler.MipFilter == "FILTER_TYPE_POINT" )
	assert( Sampler.AddressU == "TEXTURE_ADDRESS_WRAP" ) 
	assert( Sampler.AddressV == "TEXTURE_ADDRESS_MIRROR" ) 
	assert( Sampler.AddressW == "TEXTURE_ADDRESS_CLAMP" )
	assert( Sampler.MipLODBias == 2 )
	assert( Sampler.MinLOD == 0.5 )
	assert( Sampler.MaxLOD == 10 )
	assert( Sampler.MaxAnisotropy == 6 )
	assert( Sampler.ComparisonFunc == "COMPARISON_FUNC_GREATER_EQUAL" )
	assert( Sampler.BorderColor.r == 0.0 )
	assert( Sampler.BorderColor.g == 0.0 )
	assert( Sampler.BorderColor.b == 0.0 )
	assert( Sampler.BorderColor.a == 1.0 )	
	Sampler = nil
	collectgarbage()
end


function TestVertexDescInPSO(VertexDescPSO)
	CheckVertexDescription(VertexDescPSO.GraphicsPipeline.InputLayout)
	VertexDescPSO = nil
	collectgarbage()
end


function TestShaderArg(Shader)
	assert(Shader.Desc.ShaderType == "SHADER_TYPE_VERTEX")
	assert(Shader.Desc.Name == "TestVS")
	Shader = nil
	collectgarbage()
end


TestBuffer = Buffer.Create(
	{
		Name = "Test Buffer",
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_VERTEX_BUFFER", "BIND_SHADER_RESOURCE"},
		uiSizeInBytes = 64,
		Mode = "BUFFER_MODE_FORMATTED",
		Format = {ValueType = "VT_FLOAT32", NumComponents = 1, IsNormalized = false},
	},
	"VT_FLOAT32",
	{1,2,3,4,5,6,7,8, 9,10,11,12,13,14,15,16}
)
assert(TestBuffer.Name == "Test Buffer")
assert(TestBuffer.Usage == "USAGE_DEFAULT")
assert(TestBuffer.BindFlags[1] == "BIND_VERTEX_BUFFER" and TestBuffer.BindFlags[2] == "BIND_SHADER_RESOURCE" or
	   TestBuffer.BindFlags[2] == "BIND_VERTEX_BUFFER" and TestBuffer.BindFlags[1] == "BIND_SHADER_RESOURCE")
assert(TestBuffer.Mode == "BUFFER_MODE_FORMATTED")
assert(TestBuffer.Format.ValueType == "VT_FLOAT32" )
assert(TestBuffer.Format.NumComponents == 1 )
assert(TestBuffer.Format.IsNormalized == false )


function TestBufferArg (Buffer)
	assert(TestBuffer.Name == "Test Buffer")
	assert(TestBuffer.Usage == "USAGE_DEFAULT")
	assert(TestBuffer.BindFlags[1] == "BIND_VERTEX_BUFFER" and TestBuffer.BindFlags[2] == "BIND_SHADER_RESOURCE" or
		   TestBuffer.BindFlags[2] == "BIND_VERTEX_BUFFER" and TestBuffer.BindFlags[1] == "BIND_SHADER_RESOURCE")
	Buffer = nil
	collectgarbage()
end


TestBuffer2 = Buffer.Create({
	Name = "Test Buffer2",
    Usage = "USAGE_CPU_ACCESSIBLE",
	CPUAccessFlags = {"CPU_ACCESS_WRITE"},
	uiSizeInBytes = 256
})
assert(TestBuffer2.Name == "Test Buffer2")
assert(TestBuffer2.Usage == "USAGE_CPU_ACCESSIBLE")
assert(TestBuffer2.Mode == "BUFFER_MODE_UNDEFINED")
assert(TestBuffer2.CPUAccessFlags[1] == "CPU_ACCESS_WRITE")

TestBuffer3 = Buffer.Create(
	{
		Name = "Test Buffer",
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_VERTEX_BUFFER", "BIND_SHADER_RESOURCE"},
		uiSizeInBytes = 64,
		Mode = "BUFFER_MODE_FORMATTED",
		Format = {ValueType = "VT_FLOAT32", NumComponents = 1, IsNormalized = false},
	}
)

Context.SetVertexBuffers(1, TestBuffer, 0, TestBuffer3, 16, "SET_VERTEX_BUFFERS_FLAG_RESET")
Context.SetVertexBuffers(1, TestBuffer, 0, nil)
Context.SetVertexBuffers(nil, nil, nil, {"SET_VERTEX_BUFFERS_FLAG_RESET", "SET_VERTEX_BUFFERS_FLAG_RESET"})

TestBuffer3 = Buffer.Create({
	BindFlags = {"BIND_VERTEX_BUFFER", "BIND_UNORDERED_ACCESS"},
	Mode = "BUFFER_MODE_FORMATTED",
	Format = {ValueType = "VT_FLOAT32", NumComponents = 4, IsNormalized = false},
	uiSizeInBytes = (4+4) * 4 * 4,
	ElementByteStride = 4 * 4
})
assert( TestBuffer3.Mode == "BUFFER_MODE_FORMATTED" )
assert( TestBuffer3.Format.ValueType == "VT_FLOAT32" )
assert( TestBuffer3.Format.NumComponents == 4 )
assert( TestBuffer3.Format.IsNormalized == false )

Buff3DefaultUAV = TestBuffer3:GetDefaultView("BUFFER_VIEW_UNORDERED_ACCESS")
assert(Buff3DefaultUAV.ViewType == "BUFFER_VIEW_UNORDERED_ACCESS" )
assert(Buff3DefaultUAV.ByteOffset == 0 )
assert(Buff3DefaultUAV.ByteWidth == TestBuffer3.uiSizeInBytes )

Buff3UAV = TestBuffer3:CreateView{
	Name = "Buff3UAV",
	ViewType = "BUFFER_VIEW_UNORDERED_ACCESS",
	ByteOffset = 0,
	ByteWidth = TestBuffer3.ElementByteStride * 2
}

assert(Buff3UAV.Name == "Buff3UAV")
assert(Buff3UAV.ViewType == "BUFFER_VIEW_UNORDERED_ACCESS")
assert(Buff3UAV.ByteOffset == 0)
assert(Buff3UAV.ByteWidth == TestBuffer3.ElementByteStride * 2)

function TestBufferViewArg(BuffViewArg)
	assert( BuffViewArg.Name == "TestGlobalBuff2UAV" )
	assert( BuffViewArg.ViewType == "BUFFER_VIEW_UNORDERED_ACCESS" )
	assert( BuffViewArg.ByteOffset == 0 )
	assert( BuffViewArg.ByteWidth == 32 )
end


IndexBuffer = Buffer.Create(
	{ BindFlags = "BIND_INDEX_BUFFER" },
	"VT_UINT32",
	{0,1,2}
)
Context.SetIndexBuffer(IndexBuffer, 4)

TestTexture = Texture.Create{
	Name = "Test Texture 2D",
    Type = "RESOURCE_DIM_TEX_2D_ARRAY", Width = 512, Height = 512, ArraySize = 16,
    Format = "TEX_FORMAT_RGBA8_UNORM",
    MipLevels = 8,
    SampleCount = 1,
    Usage = "USAGE_DEFAULT",
    BindFlags = {"BIND_SHADER_RESOURCE", "BIND_UNORDERED_ACCESS"}
}

assert( TestTexture.Name == "Test Texture 2D" )
assert( TestTexture.Type == "RESOURCE_DIM_TEX_2D_ARRAY" )
assert( TestTexture.Width == 512 )
assert( TestTexture.Height == 512 )
assert( TestTexture.ArraySize == 16 )
assert( TestTexture.Format == "TEX_FORMAT_RGBA8_UNORM" )
assert( TestTexture.MipLevels == 8 )
assert( TestTexture.SampleCount == 1 )
assert( TestTexture.Usage == "USAGE_DEFAULT" )
assert( TestTexture.BindFlags[1] == "BIND_SHADER_RESOURCE"  and TestTexture.BindFlags[2] == "BIND_UNORDERED_ACCESS" or 
		TestTexture.BindFlags[1] == "BIND_UNORDERED_ACCESS" and TestTexture.BindFlags[2] == "BIND_SHADER_RESOURCE" )

DefaultTestSRV = TestTexture:GetDefaultView("TEXTURE_VIEW_SHADER_RESOURCE")
assert( DefaultTestSRV.ViewType == "TEXTURE_VIEW_SHADER_RESOURCE" )
assert( DefaultTestSRV.Format == TestTexture.Format )
assert( DefaultTestSRV.TextureDim == TestTexture.Type )
assert( DefaultTestSRV.MostDetailedMip == 0 )
assert( DefaultTestSRV.NumMipLevels == TestTexture.MipLevels )
assert( DefaultTestSRV.FirstArraySlice == 0 )
assert( DefaultTestSRV.NumArraySlices == TestTexture.ArraySize )

TestUAV1 = TestTexture:CreateView{
	ViewType = "TEXTURE_VIEW_UNORDERED_ACCESS",
	TextureDim = "RESOURCE_DIM_TEX_2D",
	AccessFlags = "UAV_ACCESS_FLAG_READ",
	MostDetailedMip = 1,
	NumMipLevels = 1,
	FirstArraySlice = 0,
	NumArraySlices = 1
}

assert(TestUAV1.AccessFlags[1] == "UAV_ACCESS_FLAG_READ")
assert( type(TestUAV1.AccessFlags[2]) == "nil")

TestUAV2 = TestTexture:CreateView{
	ViewType = "TEXTURE_VIEW_UNORDERED_ACCESS",
	TextureDim = "RESOURCE_DIM_TEX_2D_ARRAY",
	AccessFlags = "UAV_ACCESS_FLAG_WRITE",
	FirstArraySlice = 0,
	NumArraySlices = TestTexture.ArraySize
}
assert(TestUAV2.AccessFlags[1] == "UAV_ACCESS_FLAG_WRITE")
assert( type(TestUAV2.AccessFlags[2]) == "nil")

TestUAV3 = TestTexture:CreateView{
	ViewType = "TEXTURE_VIEW_UNORDERED_ACCESS",
	TextureDim = "RESOURCE_DIM_TEX_2D_ARRAY",
	AccessFlags = {"UAV_ACCESS_FLAG_READ", "UAV_ACCESS_FLAG_WRITE"},
	MostDetailedMip = 4,
	NumArraySlices = TestTexture.ArraySize
}

function Set (list)
  local set = {}
  for _, l in ipairs(list) do set[l] = true end
  return set
end
FlagsSet = Set(TestUAV3.AccessFlags)
assert(FlagsSet["UAV_ACCESS_FLAG_READ"] and FlagsSet["UAV_ACCESS_FLAG_WRITE"] and FlagsSet["UAV_ACCESS_FLAG_READ_WRITE"])
assert( type(TestUAV3.AccessFlags[4]) == "nil")


DefaultTestSRV:SetSampler(TestSampler)

if Constants.DeviceType == "D3D11" or Constants.DeviceType == "D3D12" then

	--TestTextureView = TextureView.Create(TestTexture,
	TestTextureView = TestTexture:CreateView( 
	{
		Name = "TestTextureSRV",
		ViewType = "TEXTURE_VIEW_SHADER_RESOURCE",
		TextureDim = "RESOURCE_DIM_TEX_2D_ARRAY",
		Format = "TEX_FORMAT_RGBA8_UNORM",
		MostDetailedMip = 1,
		NumMipLevels = 2,
		FirstArraySlice = 3,
		NumArraySlices = 4
	}
	)
	assert( TestTextureView.Name == "TestTextureSRV" )
	assert( TestTextureView.ViewType == "TEXTURE_VIEW_SHADER_RESOURCE" )
	assert( TestTextureView.TextureDim == "RESOURCE_DIM_TEX_2D_ARRAY" )
	assert( TestTextureView.Format == "TEX_FORMAT_RGBA8_UNORM" )
	assert( TestTextureView.MostDetailedMip == 1 )
	assert( TestTextureView.NumMipLevels == 2 )
	assert( TestTextureView.FirstArraySlice == 3 )
	assert( TestTextureView.NumArraySlices == 4 )

	assert( TestGlobalTextureView.Name == "TestTextureSRV2" )
	assert( TestGlobalTextureView.ViewType == "TEXTURE_VIEW_SHADER_RESOURCE" )
	assert( TestGlobalTextureView.TextureDim == "RESOURCE_DIM_TEX_2D" )
	assert( TestGlobalTextureView.Format == "TEX_FORMAT_RGBA8_UNORM" )
	assert( TestGlobalTextureView.MostDetailedMip == 0 )
	assert( TestGlobalTextureView.NumMipLevels == 1 )
	assert( TestGlobalTextureView.FirstArraySlice == 0 )
	assert( TestGlobalTextureView.NumArraySlices == 1 )
end

function TestTextureViewArg(TextureView)
	assert( TextureView.Name == "TestTextureSRV" )
	assert( TextureView.ViewType == "TEXTURE_VIEW_SHADER_RESOURCE" )
	assert( TextureView.TextureDim == "RESOURCE_DIM_TEX_2D_ARRAY" )
	assert( TextureView.Format == "TEX_FORMAT_RGBA8_UNORM" )
	assert( TextureView.MostDetailedMip == 1 )
	assert( TextureView.NumMipLevels == 2 )
	assert( TextureView.FirstArraySlice == 3 )
	assert( TextureView.NumArraySlices == 4 )
end


function TestTextureArg(Texture)
	CheckTextureAttribs( Texture )
	collectgarbage()
end


TestDrawAttribs = DrawAttribs.Create{
	NumIndices = 128,
	IndexType = "VT_UINT16",
	IsIndexed = true,
	NumInstances = 32,
	IsIndirect = true,
	BaseVertex = 48,
	IndirectDrawArgsOffset = 1024,
	StartVertexLocation = 64,
	FirstInstanceLocation = 96,
	pIndirectDrawAttribs = TestBuffer2
}
assert( TestDrawAttribs.NumIndices == 128 )
assert( TestDrawAttribs.IndexType == "VT_UINT16" )
assert( TestDrawAttribs.IsIndexed == true )
assert( TestDrawAttribs.NumInstances == 32 )
assert( TestDrawAttribs.IsIndirect == true )
assert( TestDrawAttribs.BaseVertex == 48 )
assert( TestDrawAttribs.IndirectDrawArgsOffset == 1024 )
assert( TestDrawAttribs.StartVertexLocation == 64 )
assert( TestDrawAttribs.FirstInstanceLocation == 96 )
--Context.Draw(TestBuffer2)


TestDrawAttribs.NumVertices = 92
TestDrawAttribs.IndexType = "VT_UINT32"
TestDrawAttribs.IsIndexed = false
TestDrawAttribs.NumInstances = 19
TestDrawAttribs.IsIndirect = false
TestDrawAttribs.BaseVertex = 498
TestDrawAttribs.IndirectDrawArgsOffset = 234
TestDrawAttribs.FirstIndexLocation = 264
TestDrawAttribs.FirstInstanceLocation = 946
TestDrawAttribs.pIndirectDrawAttribs = TestGlobalBuffer

TestDrawAttribs2 = DrawAttribs.Create{
	pIndirectDrawAttribs = TestDrawAttribs.pIndirectDrawAttribs
}
assert( TestDrawAttribs.pIndirectDrawAttribs.Name == TestDrawAttribs2.pIndirectDrawAttribs.Name )

assert( TestDrawAttribs.NumIndices == 92 )
assert( TestDrawAttribs.NumVertices == 92 )
assert( TestDrawAttribs.IndexType == "VT_UINT32" )
assert( TestDrawAttribs.IsIndexed == false )
assert( TestDrawAttribs.NumInstances == 19 )
assert( TestDrawAttribs.IsIndirect == false )
assert( TestDrawAttribs.BaseVertex == 498 )
assert( TestDrawAttribs.IndirectDrawArgsOffset == 234 )
assert( TestDrawAttribs.FirstIndexLocation == 264 )
assert( TestDrawAttribs.StartVertexLocation == 264 )
assert( TestDrawAttribs.FirstInstanceLocation == 946 )



assert( TestGlobalDrawAttribs.NumVertices == 123 );
assert( TestGlobalDrawAttribs.NumIndices == 123 );
assert( TestGlobalDrawAttribs.IndexType == "VT_UINT16" );
assert( TestGlobalDrawAttribs.IsIndexed == true );
assert( TestGlobalDrawAttribs.NumInstances == 19 );
assert( TestGlobalDrawAttribs.IsIndirect == true );
assert( TestGlobalDrawAttribs.BaseVertex == 97 );
assert( TestGlobalDrawAttribs.IndirectDrawArgsOffset == 120 );
assert( TestGlobalDrawAttribs.StartVertexLocation == 98 );
assert( TestGlobalDrawAttribs.FirstIndexLocation == 98 );

function TestDrawAttribsArg(DrawAttrs)
	assert( DrawAttrs.NumVertices == 34 );
	assert( DrawAttrs.NumIndices == 34 );
	assert( DrawAttrs.IndexType == "VT_UINT16" );
	assert( DrawAttrs.IsIndexed == true );
	assert( DrawAttrs.NumInstances == 139 );
	assert( DrawAttrs.IsIndirect == true );
	assert( DrawAttrs.BaseVertex == 937 );
	assert( DrawAttrs.IndirectDrawArgsOffset == 1205 );
	assert( DrawAttrs.StartVertexLocation == 198 );
	assert( DrawAttrs.FirstIndexLocation == 198 );
end
--getmetatable(TestSampler).__index = {}

TestResourceMapping = ResourceMapping.Create{
	{Name = "TestShaderName", pObject = DefaultTestSRV},
	{Name = "TestShaderName2", pObject = DefaultTestSRV},
	{Name = "TestBufferName", pObject = TestGlobalBuffer}
}

assert( TestResourceMapping["TestShaderName"].ViewType == "TEXTURE_VIEW_SHADER_RESOURCE" )
assert( TestResourceMapping["TestBufferName"].Name == "TestGlobalBuff" )
assert( type(TestResourceMapping["BadName"]) == "nil" )

TestResourceMapping["TestBufferName2"] = TestBuffer
assert( TestResourceMapping["TestBufferName2"].Name == "Test Buffer" )

TestResourceMapping["TestShaderName"] = DefaultTestSRV
assert( TestResourceMapping["TestShaderName"].ViewType == "TEXTURE_VIEW_SHADER_RESOURCE" )

TestResourceMapping["TestBufferName2"] = nil
assert( type(TestResourceMapping["TestBufferName2"]) == "nil" )

assert( TestGlobalResourceMapping["TestGlobalTextureSRV"].ViewType == "TEXTURE_VIEW_SHADER_RESOURCE" )

function TestResourceMappingArg(ResMappingArg)
	assert( ResMappingArg["TestGlobalTextureSRV2"].ViewType == "TEXTURE_VIEW_SHADER_RESOURCE" )
	ResMappingArg["TestBufferName"] = TestBuffer
end
