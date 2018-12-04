-- Render target test script


RTWidth = 256
RTHeight = 256
TestTexture0 = Texture.Create{
    Type = "RESOURCE_DIM_TEX_2D", Width = RTWidth, Height = RTHeight,
    Format = "TEX_FORMAT_RGBA8_UNORM",
    MipLevels = 1,
    SampleCount = 1,
    Usage = "USAGE_DEFAULT",
    BindFlags = {"BIND_SHADER_RESOURCE", "BIND_RENDER_TARGET"},
	ClearValue = {Color = {r=0.25, g=0.0, b=0.0, a=0.0}}
}

TestTexture1 = Texture.Create{
    Type = "RESOURCE_DIM_TEX_2D", Width = RTWidth*2, Height = RTHeight*2,
    Format = "TEX_FORMAT_RGBA32_FLOAT", -- Note that GLES3.1 only requires signed or unsigned normalized
									    -- fixed-point or signed or unsigned integer render target formats
    MipLevels = 8,
    SampleCount = 1,
    Usage = "USAGE_DEFAULT",
    BindFlags = {"BIND_SHADER_RESOURCE", "BIND_RENDER_TARGET"},
	ClearValue = {Color = {r=0.0, g=0.5, b=0.0, a=0.0}}
}

TestTexture2 = Texture.Create{
    Type = "RESOURCE_DIM_TEX_2D", Width = RTWidth, Height = RTHeight,
    Format = "TEX_FORMAT_RGBA16_FLOAT", -- Note that GLES3.1 only requires signed or unsigned normalized
									    -- fixed-point or signed or unsigned integer render target formats
    MipLevels = 1,
    SampleCount = 1,
    Usage = "USAGE_DEFAULT",
    BindFlags = {"BIND_SHADER_RESOURCE", "BIND_RENDER_TARGET"},
	ClearValue = {Color = {r=0.0, g=0.0, b=0.75, a=0.0}}
}

TestDepthTexture = Texture.Create{
    Type = "RESOURCE_DIM_TEX_2D", Width = RTWidth, Height = RTHeight,
    Format = "TEX_FORMAT_D32_FLOAT",
    MipLevels = 1,
    SampleCount = 1,
    Usage = "USAGE_DEFAULT",
    BindFlags = {"BIND_SHADER_RESOURCE", "BIND_DEPTH_STENCIL"},
	ClearValue = {DepthStencil = {Depth=1.0}}
}

IsGLES = Constants.DeviceType == "OpenGLES"

TexDesc = {
    Type = "RESOURCE_DIM_TEX_2D", Width = RTWidth, Height = RTHeight,
    Format = "TEX_FORMAT_RGBA8_UNORM",
    MipLevels = 7,
    Usage = "USAGE_DEFAULT",
    BindFlags = {"BIND_SHADER_RESOURCE", "BIND_RENDER_TARGET"},
	ClearValue = {Color = {r=0.25, g=0.5, b=0.75, a=1.0}}
} 
Tex2D = {Texture.Create(TexDesc), Texture.Create(TexDesc), Texture.Create(TexDesc) }

TexDesc.BindFlags = {"BIND_SHADER_RESOURCE", "BIND_DEPTH_STENCIL"}
TexDesc.Format = "TEX_FORMAT_D32_FLOAT"
TexDS_2D = Texture.Create(TexDesc)

TexDesc.BindFlags = {"BIND_SHADER_RESOURCE", "BIND_RENDER_TARGET"}
TexDesc.Format = "TEX_FORMAT_RGBA8_UNORM"
TexDesc.Type = "RESOURCE_DIM_TEX_2D_ARRAY"
TexDesc.ArraySize = 16
Tex2DArr = {Texture.Create(TexDesc), Texture.Create(TexDesc), Texture.Create(TexDesc) }

TexDesc.BindFlags = {"BIND_SHADER_RESOURCE", "BIND_DEPTH_STENCIL"}
TexDesc.Format = "TEX_FORMAT_D32_FLOAT"
TexDS_2DArr = Texture.Create(TexDesc)

if not IsGLES then
	TexDesc.BindFlags = {"BIND_SHADER_RESOURCE", "BIND_RENDER_TARGET"}
	TexDesc.Format = "TEX_FORMAT_RGBA8_UNORM"
	TexDesc.Type = "RESOURCE_DIM_TEX_3D"
	TexDesc.Depth = 16
	Tex3D= {Texture.Create(TexDesc), Texture.Create(TexDesc), Texture.Create(TexDesc) }

	-- Texture 3D cannot be used as depth stencil (at least in D3D11)
end

Tex2D_DefRTV = {}
Tex2D_Mip1RTV = {}
TexDS_2D_DefDSV = {}
TexDS_2D_Mip1DSV = {}

Tex2DArr_DefRTV = {}
Tex2DArr_Mip1RTV = {}
Tex2DArr_Slice2RTV = {}
Tex2DArr_Slice2Mip2RTV = {}

TexDS_2DArr_DefDSV = {}
TexDS_2DArr_Mip1DSV = {}
TexDS_2DArr_Slice2DSV = {}
TexDS_2DArr_Slice2Mip2DSV = {}

if not IsGLES then
	Tex3D_DefRTV = {}
	Tex3D_Mip1RTV = {}
	Tex3D_Slice2RTV = {}
	Tex3D_Slice2Mip2RTV = {}
end

for i = 1,3 do
	Tex2D_DefRTV[i] = Tex2D[i]:GetDefaultView("TEXTURE_VIEW_RENDER_TARGET")
	Tex2DArr_DefRTV[i] = Tex2DArr[i]:GetDefaultView("TEXTURE_VIEW_RENDER_TARGET")
	
	Tex2D_Mip1RTV[i] = Tex2D[i]:CreateView{ViewType = "TEXTURE_VIEW_RENDER_TARGET", MostDetailedMip = 1}
	Tex2DArr_Mip1RTV[i] = Tex2DArr[i]:CreateView{ViewType = "TEXTURE_VIEW_RENDER_TARGET", MostDetailedMip = 1}
	Tex2DArr_Slice2RTV[i] = Tex2DArr[i]:CreateView{ViewType = "TEXTURE_VIEW_RENDER_TARGET", FirstArraySlice = 2, NumArraySlices = 1}
	Tex2DArr_Slice2Mip2RTV[i] = Tex2DArr[i]:CreateView{ViewType = "TEXTURE_VIEW_RENDER_TARGET", FirstArraySlice = 2, MostDetailedMip = 2, NumArraySlices = 1}

	if not IsGLES then
		Tex3D_DefRTV[i] = Tex3D[i]:GetDefaultView("TEXTURE_VIEW_RENDER_TARGET")
		Tex3D_Mip1RTV[i] = Tex3D[i]:CreateView{ViewType = "TEXTURE_VIEW_RENDER_TARGET", MostDetailedMip = 1}
		Tex3D_Slice2RTV[i] = Tex3D[i]:CreateView{ViewType = "TEXTURE_VIEW_RENDER_TARGET", FirstDepthSlice = 2, NumDepthSlices = 1}
		Tex3D_Slice2Mip2RTV[i] = Tex3D[i]:CreateView{ViewType = "TEXTURE_VIEW_RENDER_TARGET", FirstDepthSlice = 2, MostDetailedMip = 2, NumDepthSlices = 1}
	end
end


Tex2D_DefDSV = TexDS_2D:GetDefaultView("TEXTURE_VIEW_DEPTH_STENCIL")
Tex2DArr_DefDSV = TexDS_2DArr:GetDefaultView("TEXTURE_VIEW_DEPTH_STENCIL")

	
Tex2D_Mip1DSV = TexDS_2D:CreateView{ViewType = "TEXTURE_VIEW_DEPTH_STENCIL", MostDetailedMip = 1}
Tex2DArr_Mip1DSV = TexDS_2DArr:CreateView{ViewType = "TEXTURE_VIEW_DEPTH_STENCIL", MostDetailedMip = 1}
Tex2DArr_Slice2DSV = TexDS_2DArr:CreateView{ViewType = "TEXTURE_VIEW_DEPTH_STENCIL", FirstArraySlice = 2, NumArraySlices = 1}
Tex2DArr_Slice2Mip2DSV = TexDS_2DArr:CreateView{ViewType = "TEXTURE_VIEW_DEPTH_STENCIL", FirstArraySlice = 2, MostDetailedMip = 2, NumArraySlices = 1}

function SetRTsHelper(RTVs, DSV)
	Context.SetRenderTargets(RTVs[1], "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	Context.SetRenderTargets(RTVs[1], RTVs[2], "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	Context.SetRenderTargets(RTVs[1], RTVs[2], RTVs[3], "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	if DSV then
		Context.SetRenderTargets(RTVs[1], RTVs[2], RTVs[3], DSV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	end
	Context.ClearRenderTarget(RTVs[1], 0.25, 0.5, 0.75, 1.0, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	Context.ClearRenderTarget(RTVs[2], 0.25, 0.5, 0.75, 1.0, "RESOURCE_STATE_TRANSITION_MODE_VERIFY")
	Context.ClearRenderTarget(RTVs[3], 0.25, 0.5, 0.75, 1.0, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	if DSV then
		Context.ClearDepthStencil(DSV, "CLEAR_DEPTH_FLAG", 1.0, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
		Context.SetRenderTargets(RTVs[1], RTVs[2], DSV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
		Context.SetRenderTargets(RTVs[1], DSV, "RESOURCE_STATE_TRANSITION_MODE_VERIFY")
		Context.SetRenderTargets(DSV, "RESOURCE_STATE_TRANSITION_MODE_VERIFY")
	end
end

function TestSetRenderTargets()
	SetRTsHelper(Tex2D_DefRTV, Tex2D_DefDSV)
	if not IsGLES then
		SetRTsHelper(Tex2DArr_DefRTV, Tex2DArr_DefDSV) -- no glFramebufferTexture in ES3.1
		SetRTsHelper(Tex3D_DefRTV)	   -- no glFramebufferTexture in ES3.1
	end

	SetRTsHelper(Tex2D_Mip1RTV, Tex2D_Mip1DSV)
	if not IsGLES then
		SetRTsHelper(Tex2DArr_Mip1RTV, Tex2DArr_Mip1DSV) -- no glFramebufferTexture in ES3.1
		SetRTsHelper(Tex3D_Mip1RTV)		 -- no glFramebufferTexture in ES3.1
	end

	SetRTsHelper(Tex2DArr_Slice2RTV, Tex2DArr_Slice2DSV)
	if not IsGLES then
		SetRTsHelper(Tex3D_Slice2RTV)	-- no glFramebufferTexture3D in ES3.1
	end

	SetRTsHelper(Tex2DArr_Slice2Mip2RTV, Tex2DArr_Slice2Mip2DSV)
	if not IsGLES then
		SetRTsHelper(Tex3D_Slice2Mip2RTV) -- no glFramebufferTexture3D in ES3.1
	end

	-- It is not allowed to mix different resources as render targets. In particular, we cannot
	-- bind Texture2D & Texture3D at the same time
	if not IsGLES then
		Context.SetRenderTargets(Tex2D_DefRTV[3], Tex2DArr_Slice2RTV[1], Tex2DArr_Slice2DSV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
		Context.SetRenderTargets(Tex2DArr_Slice2Mip2RTV[1], Tex2DArr_Slice2Mip2DSV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	end
end


Positions = { 0.0,  0.0, 0.0,
			  0.0,  1.0, 0.0,
			  1.0,  0.0, 0.0, 
			  1.0,  1.0, 0.0 }
FullQuad = {}
for v = 0,3 do
	FullQuad[v*3+1] = Positions[v*3+1]*2 - 1
	FullQuad[v*3+2] = Positions[v*3+2]*2 - 1
	FullQuad[v*3+3] = 0
end

FullQuadBuffer = Buffer.Create(
	{ BindFlags = "BIND_VERTEX_BUFFER" },
	"VT_FLOAT32",
	FullQuad
)

ScaledQuad = {}
for v = 0,3 do
	ScaledQuad[v*3+1] = Positions[v*3+1]*XExt + MinX
	ScaledQuad[v*3+2] = Positions[v*3+2]*YExt + MinY
	ScaledQuad[v*3+3] = 0
end

ScaledQuadBuffer = Buffer.Create(
	{ BindFlags = "BIND_VERTEX_BUFFER" },
	"VT_FLOAT32",
	ScaledQuad
)

TexcoordBuffer = Buffer.Create(
	{ BindFlags = "BIND_VERTEX_BUFFER" },
	"VT_FLOAT32",
	{ 0.0,  1.0,
	  0.0,  0.0,
	  1.0,  1.0, 
	  1.0,  0.0}
)


Level0Sampler = Sampler.Create{
    MinFilter = "FILTER_TYPE_LINEAR", 
    MagFilter = "FILTER_TYPE_LINEAR", 
    MipFilter = "FILTER_TYPE_POINT",
	MaxLOD = 0
}

Level1Sampler = Sampler.Create{
    MinFilter = "FILTER_TYPE_LINEAR", 
    MagFilter = "FILTER_TYPE_LINEAR", 
    MipFilter = "FILTER_TYPE_POINT",
	MaxLOD = 1,
	MinLOD = 1
}

Tex0SRV = TestTexture0:GetDefaultView("TEXTURE_VIEW_SHADER_RESOURCE")
Tex0SRV:SetSampler(Level0Sampler)
Tex0RTV = TestTexture0:GetDefaultView("TEXTURE_VIEW_RENDER_TARGET")

Tex1SRV = TestTexture1:GetDefaultView("TEXTURE_VIEW_SHADER_RESOURCE")
Tex1SRV:SetSampler(Level1Sampler)
Tex1RTV = TestTexture1:CreateView{
	ViewType = "TEXTURE_VIEW_RENDER_TARGET",
	MostDetailedMip = 1,
	}

Tex2SRV = TestTexture2:GetDefaultView("TEXTURE_VIEW_SHADER_RESOURCE")
Tex2SRV:SetSampler(Level0Sampler)
Tex2RTV = TestTexture2:GetDefaultView("TEXTURE_VIEW_RENDER_TARGET")

ResMapping = ResourceMapping.Create{
	{Name = "g_tex2DTest0", pObject = Tex0SRV},
	{Name = "g_tex2DTest1", pObject = Tex1SRV},
	{Name = "g_tex2DTest2", pObject = Tex2SRV}
}

TexDepthTexDSV = TestDepthTexture:GetDefaultView("TEXTURE_VIEW_DEPTH_STENCIL")


function GetShaderPath( ShaderName )
	
	local ProcessedShaderPath = ""
	if Constants.DeviceType == "D3D11" or Constants.DeviceType == "D3D12" then
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "_DX.hlsl"
	else
		ProcessedShaderPath = "Shaders\\" .. ShaderName .. "_GL.glsl"
	end

	return ProcessedShaderPath
end


QuadVS = Shader.Create{
	FilePath =  GetShaderPath("RTTest\\QuadVS"),
    UseCombinedTextureSamplers = true,
	Desc = {ShaderType = "SHADER_TYPE_VERTEX"}
}


RenderToTexturesPS = Shader.Create{
	FilePath =  GetShaderPath("RTTest\\RenderToTexturesPS"),
    UseCombinedTextureSamplers = true,
	Desc = {ShaderType = "SHADER_TYPE_PIXEL"}
}

BlendTexturesPS = Shader.Create{
	FilePath =  GetShaderPath("RTTest\\BlendTexturesPS"),
    UseCombinedTextureSamplers = true,
	Desc = 
	{
		ShaderType = "SHADER_TYPE_PIXEL",
		StaticSamplers = 
		{
			{
				SamplerOrTextureName = "g_tex2DTest0",
				Desc = 
				{
					MinFilter = "FILTER_TYPE_LINEAR", 
					MagFilter = "FILTER_TYPE_LINEAR", 
					MipFilter = "FILTER_TYPE_POINT",
					MaxLOD = 0
				}
			},
			{
				SamplerOrTextureName = "g_tex2DTest2",
				Desc = 
				{
					MinFilter = "FILTER_TYPE_LINEAR", 
					MagFilter = "FILTER_TYPE_LINEAR", 
					MipFilter = "FILTER_TYPE_POINT",
					MaxLOD = 0
				}
			}
		}
	}
}
assert(BlendTexturesPS.Desc.StaticSamplers[1].Desc.MipFilter == "FILTER_TYPE_POINT");
assert(BlendTexturesPS.Desc.StaticSamplers[2].SamplerOrTextureName == "g_tex2DTest2");

QuadVS:BindResources(ResMapping)
RenderToTexturesPS:BindResources(ResMapping)
BlendTexturesPS:BindResources(ResMapping)


RenderToTexPSO = PipelineState.Create
{
	Name = "Render to textures PSO",
	GraphicsPipeline = 
	{
		RasterizerDesc = 
		{
			FillMode = "FILL_MODE_SOLID",
			CullMode = "CULL_MODE_NONE",
			ScissorEnable = true
		},
		DepthStencilDesc = {DepthEnable = false},
		pVS = QuadVS,
		pPS = RenderToTexturesPS,
		InputLayout = 
		{
			{ InputIndex = 0, BufferSlot = 0, NumComponents = 3, ValueType = "VT_FLOAT32"},
			{ InputIndex = 1, BufferSlot = 1, NumComponents = 2, ValueType = "VT_FLOAT32"}
		},
		RTVFormats = {TestTexture0.Format, TestTexture1.Format},
        PrimitiveTopology = "PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP"
	}
}
RenderToTexSRB = RenderToTexPSO:CreateShaderResourceBinding(true)

BlendTexPSO = PipelineState.Create
{
	Name = "Blend textures PSO",
	GraphicsPipeline = 
	{
		RasterizerDesc = 
		{
			FillMode = "FILL_MODE_SOLID",
			CullMode = "CULL_MODE_NONE"
		},
		DepthStencilDesc = {DepthEnable = false},
		pVS = QuadVS,
		pPS = BlendTexturesPS,
		InputLayout = 
		{
			{ InputIndex = 0, BufferSlot = 0, NumComponents = 3, ValueType = "VT_FLOAT32"},
			{ InputIndex = 1, BufferSlot = 1, NumComponents = 2, ValueType = "VT_FLOAT32"}
		},
		RTVFormats = {extBackBufferFormat},
        DSVFormat = extDepthBufferFormat,
        PrimitiveTopology = "PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP"
	}
}
BlendTexSRB = BlendTexPSO:CreateShaderResourceBinding(true)

DrawAttrs = DrawAttribs.Create{
	NumVertices = 4,
    Flags = "DRAW_FLAG_VERIFY_STATES"
}

VP = Viewport.Create { 
	TopLeftX = 16,
    TopLeftY = 32,
    Width  = RTWidth-16-8,
	Height = RTHeight-32-16,
    MinDepth = 0,
    MaxDepth = 1	
}

SR = ScissorRect.Create
{
	left = 32,
	top = 48,
	right = RTWidth-48,
	bottom = RTHeight
}


function RenderToTextures()
	Context.SetVertexBuffers(FullQuadBuffer, TexcoordBuffer, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION", "SET_VERTEX_BUFFERS_FLAG_RESET")
	Context.SetPipelineState(RenderToTexPSO)
	Context.TransitionShaderResources(RenderToTexPSO, RenderToTexSRB)
	Context.CommitShaderResources(RenderToTexSRB)
	Context.Draw(DrawAttrs)
end

function BlendTextures()
	Context.SetVertexBuffers(ScaledQuadBuffer, TexcoordBuffer, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION", "SET_VERTEX_BUFFERS_FLAG_RESET")
	Context.SetPipelineState(BlendTexPSO)
	Context.CommitShaderResources(BlendTexSRB, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	Context.Draw(DrawAttrs)
end

function Render()
	TestSetRenderTargets()
	
	Context.SetRenderTargets(Tex0RTV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")	                    -- To test FBO cache
	Context.SetRenderTargets(Tex1RTV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")		                    -- To test FBO cache
	Context.SetRenderTargets(TexDepthTexDSV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")		        -- To test FBO cache
	Context.SetRenderTargets(Tex0RTV, TexDepthTexDSV, "RESOURCE_STATE_TRANSITION_MODE_VERIFY")          -- To test FBO cache
	Context.SetRenderTargets(Tex1RTV, TexDepthTexDSV, "RESOURCE_STATE_TRANSITION_MODE_VERIFY")          -- To test FBO cache
	Context.SetRenderTargets(Tex0RTV, Tex1RTV, TexDepthTexDSV, "RESOURCE_STATE_TRANSITION_MODE_VERIFY") -- To test FBO cache


	Context.SetRenderTargets(Tex2RTV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")  
	Context.ClearRenderTarget(Tex2RTV, 0, 0, 0.75, "RESOURCE_STATE_TRANSITION_MODE_VERIFY")
	Context.SetRenderTargets(Tex0RTV, Tex1RTV, "RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	Context.ClearRenderTarget(Tex0RTV, 0.25, "RESOURCE_STATE_TRANSITION_MODE_VERIFY")
	Context.ClearRenderTarget(Tex1RTV, 0.0, 0.5, "RESOURCE_STATE_TRANSITION_MODE_VERIFY")
	Context.SetViewports(VP)
	Context.SetScissorRects(SR)
	RenderToTextures()
	
	Context.SetRenderTargets("RESOURCE_STATE_TRANSITION_MODE_TRANSITION")
	Context.SetViewports()
	BlendTextures()
end
