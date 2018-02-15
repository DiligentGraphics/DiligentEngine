-- Test script file

assert( TestGlobalVP.TopLeftX == 16.5);
assert( TestGlobalVP.TopLeftY == 24.25);
assert( TestGlobalVP.Width == 156.125);
assert( TestGlobalVP.Height == 381.625);
assert( TestGlobalVP.MinDepth == 0.25);
assert( TestGlobalVP.MaxDepth == 0.75);

TestVP = Viewport.Create { 
	TopLeftX = 15.5,
    TopLeftY = 17.25,
    Width  = 128.125,
	Height = 256.625,
    MinDepth = 0.125,
    MaxDepth = 0.875	
}

assert( TestVP.TopLeftX == 15.5 )
assert( TestVP.TopLeftY == 17.25)
assert( TestVP.Width  == 128.125)
assert( TestVP.Height == 256.625)
assert( TestVP.MinDepth == 0.125)
assert( TestVP.MaxDepth == 0.875)	

TestVP.TopLeftX = 115.5
TestVP.TopLeftY = 117.25
TestVP.Width  = 1128.125
TestVP.Height = 1256.625
TestVP.MinDepth = 0.25
TestVP.MaxDepth = 0.75

assert( TestVP.TopLeftX == 115.5 )
assert( TestVP.TopLeftY == 117.25)
assert( TestVP.Width  == 1128.125)
assert( TestVP.Height == 1256.625)
assert( TestVP.MinDepth == 0.25)
assert( TestVP.MaxDepth == 0.75)	

function SetViewports()
	Context.SetViewports(TestVP)
	Context.SetViewports(TestVP, TestGlobalVP)
	Context.SetViewports(TestVP, 1024, 768)
	Context.SetViewports(TestVP, TestGlobalVP, 1024, 768)
	Context.SetViewports()
end


assert( TestGlobalSR.left == 10);
assert( TestGlobalSR.top == 30);
assert( TestGlobalSR.right == 200);
assert( TestGlobalSR.bottom == 300);

TestSR = ScissorRect.Create { 
	left = 50,
	top = 60,
	right = 400,
	bottom = 500
}

assert( TestSR.left == 50 )
assert( TestSR.top == 60 )
assert( TestSR.right == 400 )
assert( TestSR.bottom == 500 )

TestSR.left = 55
TestSR.top = 65
TestSR.right = 405
TestSR.bottom = 505
assert( TestSR.left == 55 )
assert( TestSR.top == 65 )
assert( TestSR.right == 405 )
assert( TestSR.bottom == 505 )

function SetScissorRects()
	Context.SetScissorRects(TestSR)
	Context.SetScissorRects(TestSR, TestGlobalSR)
	Context.SetScissorRects(TestSR, 1024, 768)
	Context.SetScissorRects(TestSR, TestGlobalSR, 1280, 1024)
end
