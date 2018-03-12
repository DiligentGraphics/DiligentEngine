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
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_VERTEX_BUFFER", "BIND_UNORDERED_ACCESS"},
		Mode = "BUFFER_MODE_FORMATTED",
		Format = {ValueType = "VT_FLOAT32", NumComponents = 4, IsNormalized = false},
		uiSizeInBytes = (4 + 8) * 4 * 4
	}
)

TexcoordBuffer = Buffer.Create(
	{
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_VERTEX_BUFFER", "BIND_UNORDERED_ACCESS"},
		Mode = "BUFFER_MODE_FORMATTED",
		Format = {ValueType = "VT_FLOAT32", NumComponents = 4, IsNormalized = false},
		uiSizeInBytes = (4 + 8) * 4 * 4
	}
)

OffsetsBuffer = Buffer.Create(
	{
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_UNORDERED_ACCESS", "BIND_SHADER_RESOURCE"},
		--uiSizeInBytes = 64,
		Mode = "BUFFER_MODE_FORMATTED",
		Format = {ValueType = "VT_FLOAT32", NumComponents = 4, IsNormalized = false},
	},
	"VT_FLOAT32",
	{0.03,0,0,0,  0,0.03,0,0,  0.03,0.03,0,0,  -0.03,-0.03,0,0}
)

IndexBuffer  = Buffer.Create(
	{
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_INDEX_BUFFER", "BIND_UNORDERED_ACCESS"},
		Mode = "BUFFER_MODE_FORMATTED",
		Format = {ValueType = "VT_UINT32", NumComponents = 4, IsNormalized = false},
	},
	"VT_UINT32",
	{0,0,0,0}
)

IndirectDrawArgsBuffer = Buffer.Create(
	{
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_INDIRECT_DRAW_ARGS", "BIND_UNORDERED_ACCESS"},
		Format = {ValueType = "VT_UINT32", NumComponents = 4, IsNormalized = false},
		Mode = "BUFFER_MODE_FORMATTED",
		uiSizeInBytes = 32
	}
)

IndirectDispatchArgsBuffer =  Buffer.Create(
	{
		Usage = "USAGE_DEFAULT",
		BindFlags = {"BIND_INDIRECT_DRAW_ARGS", "BIND_UNORDERED_ACCESS"},
		Format = {ValueType = "VT_UINT32", NumComponents = 4, IsNormalized = false},
		Mode = "BUFFER_MODE_FORMATTED",
		uiSizeInBytes = 64
	}
)

if Constants.DeviceType == "D3D11" or Constants.DeviceType == "D3D12" then
	TexcoordDataOffset = 0 -- Non-zero byte offset is only supported for structured buffers in DirectX
else
	TexcoordDataOffset = 2*4*4
end

TexCoordUAV = TexcoordBuffer:CreateView{
	ViewType = "BUFFER_VIEW_UNORDERED_ACCESS",
	ByteOffset = TexcoordDataOffset,
	ByteWidth = 4*4*4
}

ResMapping = ResourceMapping.Create{
	{Name = "g_tex2DTest", pObject = TestTexture:GetDefaultView("TEXTURE_VIEW_SHADER_RESOURCE")},
	{Name = "bufPositions", pObject = PositionsBuffer:GetDefaultView("BUFFER_VIEW_UNORDERED_ACCESS") },
	{Name = "bufTexcoord", pObject = TexCoordUAV},
	{Name = "bufIndices", pObject = IndexBuffer:GetDefaultView("BUFFER_VIEW_UNORDERED_ACCESS") },
	{Name = "bufIndirectDrawArgs", pObject = IndirectDrawArgsBuffer:GetDefaultView("BUFFER_VIEW_UNORDERED_ACCESS") },
	{Name = "bufIndirectDispatchArgs", pObject = IndirectDispatchArgsBuffer:CreateView{ ViewType = "BUFFER_VIEW_UNORDERED_ACCESS"} },
	{Name = "Offsets", pObject = OffsetsBuffer:CreateView{ ViewType = "BUFFER_VIEW_SHADER_RESOURCE"} }
}

function GetShaderPath( ShaderName, ShaderExt, GLESSpecial )
	
	local ProcessedShaderPath = ""
	if Constants.DeviceType == "D3D11" or Constants.DeviceType == "D3D12" then
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "DX." .. ShaderExt
	elseif Constants.DeviceType == "OpenGL" or not GLESSpecial then
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "GL." .. ShaderExt
	else
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "GLES." .. ShaderExt
	end

	return ProcessedShaderPath
end

FillTextureCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\FillTexture", "csh"),
	Desc = 
	{
		ShaderType = "SHADER_TYPE_COMPUTE",
		VariableDesc = {{Name = "g_tex2DTestUAV", Type = "SHADER_VARIABLE_TYPE_DYNAMIC"}}
	}
}
assert(FillTextureCS.Desc.VariableDesc[1].Name == "g_tex2DTestUAV");
assert(FillTextureCS.Desc.VariableDesc[1].Type == "SHADER_VARIABLE_TYPE_DYNAMIC");

UpdateVertBuffCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\UpdateVertBuff", "csh", true),
	Desc = {ShaderType = "SHADER_TYPE_COMPUTE"}
}

UpdateIndBuffCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\UpdateIndBuff", "csh"),
	Desc = {ShaderType = "SHADER_TYPE_COMPUTE"}
}

UpdateDrawArgsBuffCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\UpdateDrawArgsBuff", "csh"),
	Desc = {ShaderType = "SHADER_TYPE_COMPUTE"}
}

UpdateDispatchArgsBuffCS = Shader.Create{
	FilePath =  GetShaderPath("CSTest\\UpdateDispatchArgsBuff", "csh"),
	Desc = {ShaderType = "SHADER_TYPE_COMPUTE"}
}

RenderVS = Shader.Create{
	FilePath =  GetShaderPath("TextureTest", "vsh"),
	Desc = { ShaderType = "SHADER_TYPE_VERTEX" }
}

RenderPS = Shader.Create{
	FilePath =  GetShaderPath("TextureTest", "psh"),
	Desc = { ShaderType = "SHADER_TYPE_PIXEL" }
}

FillTexturePSO = PipelineState.Create
{
	Name = "FillTexturePSO",
	IsComputePipeline=true,
	ComputePipeline = 
	{
		pCS = FillTextureCS
	}
}
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

UpdateIndBuffPSO = PipelineState.Create
{
	Name = "UpdateIndBuffPSO",
	IsComputePipeline=true,
	ComputePipeline = 
	{
		pCS = UpdateIndBuffCS
	}
}

UpdateDrawArgsBuffPSO = PipelineState.Create
{
	Name = "UpdateDrawArgsBuffPSO",
	IsComputePipeline=true,
	ComputePipeline = 
	{
		pCS = UpdateDrawArgsBuffCS
	}
}

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
			{ InputIndex = 0, BufferSlot = 0, NumComponents = 3, ValueType = "VT_FLOAT32"},
			{ InputIndex = 1, BufferSlot = 1, NumComponents = 2, ValueType = "VT_FLOAT32"}
		},
		RTVFormats = {"TEX_FORMAT_RGBA8_UNORM_SRGB"}
	}
}

UpdateVertBuffCS:BindResources(ResMapping, {"BIND_SHADER_RESOURCES_RESET_BINDINGS", "BIND_SHADER_RESOURCES_ALL_RESOLVED"} )
UpdateIndBuffCS:BindResources(ResMapping, {"BIND_SHADER_RESOURCES_RESET_BINDINGS", "BIND_SHADER_RESOURCES_ALL_RESOLVED"})
UpdateDrawArgsBuffCS:BindResources(ResMapping, {"BIND_SHADER_RESOURCES_RESET_BINDINGS", "BIND_SHADER_RESOURCES_ALL_RESOLVED"})
UpdateDispatchArgsBuffCS:BindResources(ResMapping, {"BIND_SHADER_RESOURCES_RESET_BINDINGS", "BIND_SHADER_RESOURCES_ALL_RESOLVED"})
RenderVS:BindResources(ResMapping, {"BIND_SHADER_RESOURCES_RESET_BINDINGS", "BIND_SHADER_RESOURCES_ALL_RESOLVED"})
RenderPS:BindResources(ResMapping, {"BIND_SHADER_RESOURCES_RESET_BINDINGS", "BIND_SHADER_RESOURCES_ALL_RESOLVED"})


DrawAttrs = DrawAttribs.Create{
    Topology = "PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP",
	IsIndexed = true,
	IndexType = "VT_UINT32",
	IsIndirect = true,
	pIndirectDrawAttribs = IndirectDrawArgsBuffer
}



function Draw()
	Context.SetPipelineState(RenderPSO)
	Context.TransitionShaderResources(RenderPSO)
	Context.CommitShaderResources()
	Context.SetVertexBuffers(PositionsBuffer, 0, 4*4, TexcoordBuffer, TexcoordDataOffset, 4*4, "SET_VERTEX_BUFFERS_FLAG_RESET")
	Context.SetIndexBuffer(IndexBuffer)
	Context.Draw(DrawAttrs)
end


tex2DTestUAV = FillTextureSRB:GetVariable("SHADER_TYPE_COMPUTE", "g_tex2DTestUAV")

function Dispatch()

	Context.SetPipelineState(FillTexturePSO)

	for Mip = 0, TestTexture.MipLevels-1 do
		local NumGroupsX = math.floor( (bit32.rshift(TestTexture.Width,  Mip) + ThreadGroupSizeX - 1) / ThreadGroupSizeX )
		local NumGroupsY = math.floor( (bit32.rshift(TestTexture.Height, Mip) + ThreadGroupSizeX - 1) / ThreadGroupSizeY )
		tex2DTestUAV:Set(UAVs[Mip])
		
		Context.CommitShaderResources(FillTextureSRB, "COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES" )

		Context.DispatchCompute(NumGroupsX, NumGroupsY)	
	end

	Context.SetPipelineState(UpdateDispatchArgsBuffPSO)
	Context.TransitionShaderResources(UpdateDispatchArgsBuffPSO)
	Context.CommitShaderResources()
	Context.DispatchCompute(1)

	Context.SetPipelineState(UpdateDrawArgsBuffPSO)
	Context.CommitShaderResources("COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES")
	Context.DispatchCompute(IndirectDispatchArgsBuffer, 16)

	Context.SetPipelineState(UpdateIndBuffPSO)
	Context.TransitionShaderResources(UpdateIndBuffPSO)
	Context.CommitShaderResources()
	Context.DispatchCompute(1)
	
	Context.SetPipelineState(UpdateVertBuffPSO)
	Context.CommitShaderResources("COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES")
	Context.DispatchCompute(2)	

	return Draw()
end

function Render()
	return Dispatch()
end
