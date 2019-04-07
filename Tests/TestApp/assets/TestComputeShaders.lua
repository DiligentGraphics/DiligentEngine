-- Compute shader test script

ThreadGroupSizeX = 16
ThreadGroupSizeY = 8

TestTexture = Texture.Create{
	Name = "Compute Shader Test Texture",
    Type = "RESOURCE_DIM_TEX_2D", Width = 256, Height = 256,
    Format = "TEX_FORMAT_RGBA8_UNORM",
    MipLevels = 7,
    SampleCount = 1,
    Usage = "USAGE_DEFAULT",
    BindFlags = {"BIND_SHADER_RESOURCE", "BIND_UNORDERED_ACCESS"}
}

UAVs = { [0] = TestTexture:CreateView{ ViewType = "TEXTURE_VIEW_UNORDERED_ACCESS", AccessFlags = "UAV_ACCESS_FLAG_WRITE"} }
for mip = 1, TestTexture.MipLevels-1 do
	UAVs[mip] = TestTexture:CreateView{ ViewType = "TEXTURE_VIEW_UNORDERED_ACCESS", MostDetailedMip = mip, AccessFlags = "UAV_ACCESS_FLAG_WRITE" }
	assert(UAVs[mip].AccessFlags[1] == "UAV_ACCESS_FLAG_WRITE" )
end

LinearSampler = Sampler.Create{
    MinFilter = "FILTER_TYPE_LINEAR", 
    MagFilter = "FILTER_TYPE_LINEAR", 
    MipFilter = "FILTER_TYPE_LINEAR"
}

TestTexture:GetDefaultView("TEXTURE_VIEW_SHADER_RESOURCE"):SetSampler(LinearSampler)

PositionsBuffer = Buffer.Create(
	{
        Name = "Position buffer",
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_VERTEX_BUFFER", "BIND_UNORDERED_ACCESS"},
		Mode = "BUFFER_MODE_FORMATTED",
        ElementByteStride = 16,
		uiSizeInBytes = (4 + 8) * 4 * 4
	}
)
PositionsBufferUAV = PositionsBuffer:CreateView
{
    Name = "bufPositions UAV",
    ViewType = "BUFFER_VIEW_UNORDERED_ACCESS",
    Format = {ValueType = "VT_FLOAT32", NumComponents = 4}
}


TexcoordBuffer = Buffer.Create(
	{
        Name = "Texcoord buffer",
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_VERTEX_BUFFER", "BIND_UNORDERED_ACCESS"},
		Mode = "BUFFER_MODE_FORMATTED",
        ElementByteStride = 16,
		uiSizeInBytes = (4 + 8) * 4 * 4
	}
)



OffsetsBuffer = Buffer.Create(
	{
        Name = "Offsets buffer",
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_UNORDERED_ACCESS", "BIND_SHADER_RESOURCE"},
		--uiSizeInBytes = 64,
		Mode = "BUFFER_MODE_FORMATTED",
        ElementByteStride = 16
	},
	"VT_FLOAT32",
	{0.03,0,0,0,  0,0.03,0,0,  0.03,0.03,0,0,  -0.03,-0.03,0,0}
)
OffsetsBufferSRV = OffsetsBuffer:CreateView
{
    Name = "Offsets UAV",
    ViewType = "BUFFER_VIEW_SHADER_RESOURCE",
    Format = {ValueType = "VT_FLOAT32", NumComponents = 4}
}


IndexBuffer  = Buffer.Create(
	{
        Name = "Index buffer",
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_INDEX_BUFFER", "BIND_UNORDERED_ACCESS"},
		Mode = "BUFFER_MODE_FORMATTED",
        ElementByteStride = 16
	},
	"VT_UINT32",
	{0,0,0,0}
)
IndexBufferUAV = IndexBuffer:CreateView
{
    Name = "bufIndices UAV",
    ViewType = "BUFFER_VIEW_UNORDERED_ACCESS",
    Format = {ValueType = "VT_UINT32", NumComponents = 4}
}


IndirectDrawArgsBuffer = Buffer.Create(
	{
        Name = "Indirect Draw Args Buffer",
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_INDIRECT_DRAW_ARGS", "BIND_UNORDERED_ACCESS"},
        ElementByteStride = 16,
		Mode = "BUFFER_MODE_FORMATTED",
		uiSizeInBytes = 32
	}
)
IndirectDrawArgsBufferUAV = IndirectDrawArgsBuffer:CreateView
{
    Name = "bufIndirectDrawArgs UAV",
    ViewType = "BUFFER_VIEW_UNORDERED_ACCESS",
    Format = {ValueType = "VT_UINT32", NumComponents = 4}
}


IndirectDispatchArgsBuffer =  Buffer.Create(
	{
        Name = "Indirect Dispatch Args Buffer",
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_INDIRECT_DRAW_ARGS", "BIND_UNORDERED_ACCESS"},
        ElementByteStride = 16,
		Mode = "BUFFER_MODE_FORMATTED",
		uiSizeInBytes = 64
	}
)
IndirectDispatchArgsBufferUAV = IndirectDispatchArgsBuffer:CreateView
{
    Name = "bufIndirectDispatchArgs UAV",
    ViewType = "BUFFER_VIEW_UNORDERED_ACCESS",
    Format = {ValueType = "VT_UINT32", NumComponents = 4}
}

TexcoordDataOffset = 2*4*4
TexCoordUAV = TexcoordBuffer:CreateView
{
	ViewType = "BUFFER_VIEW_UNORDERED_ACCESS",
	ByteOffset = TexcoordDataOffset,
	ByteWidth = 4*4*4,
    Format = {ValueType = "VT_FLOAT32", NumComponents = 4}
}

ResMapping = ResourceMapping.Create{
	{Name = "g_tex2DTest", pObject = TestTexture:GetDefaultView("TEXTURE_VIEW_SHADER_RESOURCE")},
	{Name = "bufPositions", pObject = PositionsBufferUAV },
	{Name = "bufTexcoord", pObject = TexCoordUAV},
	{Name = "bufIndices", pObject = IndexBufferUAV},
	{Name = "bufIndirectDrawArgs", pObject = IndirectDrawArgsBufferUAV },
	{Name = "bufIndirectDispatchArgs", pObject = IndirectDispatchArgsBufferUAV },
	{Name = "Offsets", pObject = OffsetsBufferSRV}
}

function GetShaderPath( ShaderName, ShaderExt, GLESSpecial )
	
	local ProcessedShaderPath = ""
	if Constants.DeviceType == "D3D11" or Constants.DeviceType == "D3D12" then
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "DX." .. ShaderExt
	elseif Constants.DeviceType == "OpenGL" or Constants.DeviceType == "Vulkan" or not GLESSpecial then
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "GL." .. ShaderExt
	else
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "GLES." .. ShaderExt
	end

	return ProcessedShaderPath
end

FillTextureCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\FillTexture", "csh"),
    UseCombinedTextureSamplers = true,
	Desc = 
	{
		ShaderType = "SHADER_TYPE_COMPUTE"
	}
}

UpdateVertBuffCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\UpdateVertBuff", "csh", true),
    UseCombinedTextureSamplers = true,
	Desc = {ShaderType = "SHADER_TYPE_COMPUTE"}
}

UpdateIndBuffCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\UpdateIndBuff", "csh"),
    UseCombinedTextureSamplers = true,
	Desc = {ShaderType = "SHADER_TYPE_COMPUTE"}
}

UpdateDrawArgsBuffCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\UpdateDrawArgsBuff", "csh"),
    UseCombinedTextureSamplers = true,
	Desc = {ShaderType = "SHADER_TYPE_COMPUTE"}
}

UpdateDispatchArgsBuffCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\UpdateDispatchArgsBuff", "csh"),
    UseCombinedTextureSamplers = true,
	Desc = {ShaderType = "SHADER_TYPE_COMPUTE"}
}

RenderVS = Shader.Create{
	FilePath =  GetShaderPath("TextureTest", "vsh"),
    UseCombinedTextureSamplers = true,
	Desc = { ShaderType = "SHADER_TYPE_VERTEX" }
}

RenderPS = Shader.Create{
	FilePath =  GetShaderPath("TextureTest", "psh"),
    UseCombinedTextureSamplers = true,
	Desc = { ShaderType = "SHADER_TYPE_PIXEL" }
}

FillTexturePSO = PipelineState.Create
{
	Name = "FillTexturePSO",
    ResourceLayout = 
    {
		Variables = {{ShaderStages = "SHADER_TYPE_COMPUTE", Name = "g_tex2DTestUAV", Type = "SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC"}}
    },
	IsComputePipeline=true,
	ComputePipeline = 
	{
		pCS = FillTextureCS
	}
}
assert(FillTexturePSO.ResourceLayout.Variables[1].ShaderStages[1] == "SHADER_TYPE_COMPUTE");
assert(FillTexturePSO.ResourceLayout.Variables[1].Name == "g_tex2DTestUAV");
assert(FillTexturePSO.ResourceLayout.Variables[1].Type == "SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC");

FillTextureSRB = FillTexturePSO:CreateShaderResourceBinding()

UpdateVertBuffPSO = PipelineState.Create
{
	Name = "UpdateVertBuffPSO",
	IsComputePipeline=true,
	ComputePipeline = 
	{
		pCS = UpdateVertBuffCS
	}
}
UpdateVertBuffSRB = UpdateVertBuffPSO:CreateShaderResourceBinding()

UpdateIndBuffPSO = PipelineState.Create
{
	Name = "UpdateIndBuffPSO",
	IsComputePipeline=true,
	ComputePipeline = 
	{
		pCS = UpdateIndBuffCS
	}
}
UpdateIndBuffSRB = UpdateIndBuffPSO:CreateShaderResourceBinding()

UpdateDrawArgsBuffPSO = PipelineState.Create
{
	Name = "UpdateDrawArgsBuffPSO",
	IsComputePipeline=true,
	ComputePipeline = 
	{
		pCS = UpdateDrawArgsBuffCS
	}
}
UpdateDrawArgsBuffSRB = UpdateDrawArgsBuffPSO:CreateShaderResourceBinding()

UpdateDispatchArgsBuffPSO = PipelineState.Create
{
	Name = "UpdateDispatchArgsBuffPSO",
	IsComputePipeline=true,
	ComputePipeline = 
	{
		pCS = UpdateDispatchArgsBuffCS
	}
}
assert(UpdateDrawArgsBuffPSO:IsCompatibleWith(UpdateDispatchArgsBuffPSO) == true)
assert(UpdateDispatchArgsBuffPSO:IsCompatibleWith(UpdateDrawArgsBuffPSO) == true)
UpdateDispatchArgsBuffSRB = UpdateDispatchArgsBuffPSO:CreateShaderResourceBinding()

RenderPSO = PipelineState.Create
{
	Name = "CS Test - Render PSO",
	GraphicsPipeline = 
	{
		RasterizerDesc = 
		{
			FillMode = "FILL_MODE_SOLID",
			CullMode = "CULL_MODE_NONE"
		},
		DepthStencilDesc = {DepthEnable = false},
		pVS = RenderVS,
		pPS = RenderPS,
		InputLayout = 
		{
			{ InputIndex = 0, BufferSlot = 0, NumComponents = 3, ValueType = "VT_FLOAT32", IsNormalized = false, Stride = 4*4},
			{ InputIndex = 1, BufferSlot = 1, NumComponents = 2, ValueType = "VT_FLOAT32", IsNormalized = false, Stride = 4*4}
		},
		RTVFormats = {extBackBufferFormat},
        DSVFormat = extDepthBufferFormat,
        PrimitiveTopology = "PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP"
	}
}
RenderPSO:BindStaticResources({"SHADER_TYPE_VERTEX", "SHADER_TYPE_PIXEL"}, ResMapping, {"BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED"})
RenderSRB = RenderPSO:CreateShaderResourceBinding()

UpdateVertBuffPSO:BindStaticResources("SHADER_TYPE_COMPUTE", ResMapping, {"BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED", "BIND_SHADER_RESOURCES_UPDATE_STATIC"} )
UpdateIndBuffPSO:BindStaticResources("SHADER_TYPE_COMPUTE", ResMapping, {"BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED", "BIND_SHADER_RESOURCES_UPDATE_ALL"})
UpdateDrawArgsBuffPSO:BindStaticResources("SHADER_TYPE_COMPUTE", ResMapping, {"BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED"})
UpdateDispatchArgsBuffPSO:BindStaticResources("SHADER_TYPE_COMPUTE", ResMapping, {"BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED"})

FillTextureSRB:InitializeStaticResources()
UpdateVertBuffSRB:InitializeStaticResources()
UpdateIndBuffSRB:InitializeStaticResources()
UpdateDrawArgsBuffSRB:InitializeStaticResources()
UpdateDispatchArgsBuffSRB:InitializeStaticResources()
RenderSRB:InitializeStaticResources()

DrawAttrs = DrawAttribs.Create{
    IsIndexed = true,
	IndexType = "VT_UINT32",
	pIndirectDrawAttribs = IndirectDrawArgsBuffer,
    IndirectAttribsBufferStateTransitionMode = "RESOURCE_STATE_TRANSITION_MODE_TRANSITION",
    Flags = {"DRAW_FLAG_VERIFY_ALL"}
}



function Draw()
	Context.SetPipelineState(RenderPSO)
	Context.TransitionShaderResources(RenderPSO, RenderSRB)
	Context.CommitShaderResources(RenderSRB)
	Context.SetVertexBuffers(PositionsBuffer, 0, TexcoordBuffer, TexcoordDataOffset, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION", "SET_VERTEX_BUFFERS_FLAG_RESET")
	Context.SetIndexBuffer(IndexBuffer, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION" )
	Context.Draw(DrawAttrs)
end


tex2DTestUAV = FillTextureSRB:GetVariableByName("SHADER_TYPE_COMPUTE", "g_tex2DTestUAV")

function Dispatch()

	Context.SetPipelineState(FillTexturePSO)

	for Mip = 0, TestTexture.MipLevels-1 do
		local NumGroupsX = math.floor( (bit32.rshift(TestTexture.Width,  Mip) + ThreadGroupSizeX - 1) / ThreadGroupSizeX )
		local NumGroupsY = math.floor( (bit32.rshift(TestTexture.Height, Mip) + ThreadGroupSizeX - 1) / ThreadGroupSizeY )
		tex2DTestUAV:Set(UAVs[Mip])
		
		Context.CommitShaderResources(FillTextureSRB, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION" )

		Context.DispatchCompute(NumGroupsX, NumGroupsY)
	end

	Context.SetPipelineState(UpdateDispatchArgsBuffPSO)
	Context.TransitionShaderResources(UpdateDispatchArgsBuffPSO, UpdateDispatchArgsBuffSRB)
	Context.CommitShaderResources(UpdateDispatchArgsBuffSRB)
	Context.DispatchCompute(1)

	Context.SetPipelineState(UpdateDrawArgsBuffPSO)
	Context.CommitShaderResources(UpdateDrawArgsBuffSRB, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	Context.DispatchCompute(IndirectDispatchArgsBuffer, 16, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")

	Context.SetPipelineState(UpdateIndBuffPSO)
	Context.TransitionShaderResources(UpdateIndBuffPSO, UpdateIndBuffSRB)
	Context.CommitShaderResources(UpdateIndBuffSRB)
	Context.DispatchCompute(1)
	
	Context.SetPipelineState(UpdateVertBuffPSO)
	Context.CommitShaderResources(UpdateVertBuffSRB, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	Context.DispatchCompute(2)	

	return Draw()
end

function Render()
	return Dispatch()
end
