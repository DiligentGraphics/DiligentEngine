-- Test render scripts

TestSampler = Sampler.Create{
    Name = "Test Sampler",
    MinFilter = "FILTER_TYPE_POINT", 
    MagFilter = "FILTER_TYPE_LINEAR", 
    MipFilter = "FILTER_TYPE_POINT", 
    AddressU = "TEXTURE_ADDRESS_WRAP", 
    AddressV = "TEXTURE_ADDRESS_MIRROR", 
    AddressW = "TEXTURE_ADDRESS_CLAMP",
    MipLODBias = 2,
    MinLOD = 0.4,
    MaxLOD = 10,
    MaxAnisotropy = 6,
    ComparisonFunc = "COMPARISON_FUNC_GREATER_EQUAL",
    BorderColor = {r = 0.0, g = 0.0, b=0.0, a=1.0},
}

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

MinimalVS = Shader.Create{
	FilePath =  GetShaderPath("minimal", "vsh"),
    UseCombinedTextureSamplers = true,
	Desc = {
		ShaderType = "SHADER_TYPE_VERTEX",
		Name = "MinimalVS"
	}
}

MinimalInstVS = Shader.Create{
	FilePath =  GetShaderPath("minimalInst", "vsh"),
    UseCombinedTextureSamplers = true,
	Desc = {
		ShaderType = "SHADER_TYPE_VERTEX",
		Name = "MinimalInstVS"
	}
}

UniformBufferPS = Shader.Create{
	FilePath =  GetShaderPath("UniformBuffer", "psh"),
    UseCombinedTextureSamplers = true,
	Desc = {
		ShaderType = "SHADER_TYPE_PIXEL",
		Name = "UniformBufferPS"
	}
}

PSO = PipelineState.Create
{
	Name = "Pipeline State",
	GraphicsPipeline = 
	{
		DepthStencilDesc = 
		{
			DepthEnable = false
		},
		RasterizerDesc = 
		{
			CullMode = "CULL_MODE_NONE"
		},
		BlendDesc = 
		{
			IndependentBlendEnable = false,
			RenderTargets = { {BlendEnable = false} }
		},
		InputLayout = 
		{
			{ InputIndex = 0, BufferSlot = 0, NumComponents = 3, ValueType = "VT_FLOAT32", IsNormalized = false},
			{ InputIndex = 1, BufferSlot = 1, NumComponents = 4, ValueType = "VT_UINT8",   IsNormalized = true},
		},
		PrimitiveTopology = "PRIMITIVE_TOPOLOGY_TRIANGLE_LIST",
		pVS = MinimalVS,
		pPS = UniformBufferPS,
		RTVFormats = extBackBufferFormat,
		DSVFormat = extDepthBufferFormat
	},
	SRBAllocationGranularity = 16
}

SRB = PSO:CreateShaderResourceBinding()
SRB1 = PSO:CreateShaderResourceBinding()
SRB2 = PSO:CreateShaderResourceBinding()
SRB3 = PSO:CreateShaderResourceBinding()

PSOInst = PipelineState.Create
{
	Name = "Pipeline State",
	GraphicsPipeline = 
	{
		DepthStencilDesc = 
		{
			DepthEnable = false
		},
		RasterizerDesc = 
		{
			CullMode = "CULL_MODE_NONE"
		},
		BlendDesc = 
		{
			IndependentBlendEnable = false,
			RenderTargets = { {BlendEnable = false} }
		},
		InputLayout = 
		{
			{ InputIndex = 0, BufferSlot = 0, NumComponents = 3, ValueType = "VT_FLOAT32", IsNormalized = false, Stride = 4*3},
			{ InputIndex = 1, BufferSlot = 1, NumComponents = 4, ValueType = "VT_UINT8",   IsNormalized = true, Stride = 4*1},
			{ InputIndex = 2, BufferSlot = 2, NumComponents = 2, ValueType = "VT_FLOAT32", IsNormalized = false, Frequency = "FREQUENCY_PER_INSTANCE", Stride = 0},
		},
		PrimitiveTopology = "PRIMITIVE_TOPOLOGY_TRIANGLE_LIST",
		pVS = MinimalInstVS,
		pPS = UniformBufferPS,
		RTVFormats = extBackBufferFormat,
		DSVFormat = extDepthBufferFormat
	}
}
SRBInst = PSOInst:CreateShaderResourceBinding()

VertexBuffer1 = Buffer.Create(
	{ BindFlags = "BIND_VERTEX_BUFFER" },
	"VT_FLOAT32",
	{-1.0, 3.0, 0.0,
	 3.0, -1.0, 0.0,
	-1.0, -1.0, 0.0}
)

VertexBuffer2 = Buffer.Create(
	{ BindFlags = "BIND_VERTEX_BUFFER" },
	"VT_FLOAT32",
	{0.0, 0.0, 0.0,
	 0.0,-0.2, 0.0,
	 0.2, 0.0, 0.0}
)

ColorsBuffer1 = Buffer.Create(
	{ BindFlags = "BIND_VERTEX_BUFFER" },
	"VT_UINT8",
	{127, 0, 0, 0,
	 0, 127, 0, 0,
	0, 0, 127, 0}
)

ColorsBuffer2 = Buffer.Create(
	{ BindFlags = "BIND_VERTEX_BUFFER" },
	"VT_UINT8",
	{255, 255,   0, 0,
	 0,   255, 255, 0,
	255,   0, 255, 0 }
)

InstanceBuffer = Buffer.Create(
	{ BindFlags = "BIND_VERTEX_BUFFER" },
	"VT_FLOAT32",
	{-0.3, 0.0, 0.0, 0.0, 0.3, -0.3}
)

IndexBuffer = Buffer.Create(
	{ BindFlags = "BIND_INDEX_BUFFER" },
	"VT_UINT32",
	{0,1,2}
)

UnfiformBuffer1 = Buffer.Create(
	{ Name = "Test Uniform Buffer",  
	  uiSizeInBytes = 16,
      BindFlags = "BIND_UNIFORM_BUFFER", Usage = "USAGE_DYNAMIC", CPUAccessFlags = "CPU_ACCESS_WRITE" }
)

ResMapping = ResourceMapping.Create{
	{Name = "cbTestBlock", pObject = UnfiformBuffer1}
}


function AddConstBufferToMapping(Name, NewConstBuff)
	ResMapping[Name] = NewConstBuff
end

function BindShaderResources()
	MinimalVS:BindResources(ResMapping)
	MinimalInstVS:BindResources(ResMapping)
	UniformBufferPS:BindResources(ResMapping)
    SRB:InitializeStaticResources(PSO)
    SRBInst:InitializeStaticResources()
end


function DrawTris(DrawAttrs)
	
	if( DrawAttrs.NumInstances == 1 ) then
		Context.SetPipelineState(PSO)
		Context.SetVertexBuffers(0, VertexBuffer1, 0, ColorsBuffer1, 0, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION", "SET_VERTEX_BUFFERS_FLAG_RESET")
		SRB:BindResources({"SHADER_TYPE_VERTEX", "SHADER_TYPE_PIXEL"}, ResMapping, {"BIND_SHADER_RESOURCES_KEEP_EXISTING", "BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED", "BIND_SHADER_RESOURCES_UPDATE_ALL"})
		Context.TransitionShaderResources(PSO, SRB)
		Context.CommitShaderResources(SRB)
		Context.Draw(DrawAttrs)
	else
		Context.SetPipelineState(PSOInst)
		Context.SetVertexBuffers(0, VertexBuffer2, ColorsBuffer2, 0, InstanceBuffer, 0, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION", "SET_VERTEX_BUFFERS_FLAG_RESET")
		Context.SetIndexBuffer(IndexBuffer, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
		SRBInst:BindResources({"SHADER_TYPE_VERTEX", "SHADER_TYPE_PIXEL"}, ResMapping, {"BIND_SHADER_RESOURCES_KEEP_EXISTING", "BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED", "BIND_SHADER_RESOURCES_UPDATE_MUTABLE", "BIND_SHADER_RESOURCES_UPDATE_DYNAMIC"})
		Context.TransitionShaderResources(PSOInst, SRBInst)
		Context.CommitShaderResources(SRBInst)
		Context.Draw(DrawAttrs)
	end

end
