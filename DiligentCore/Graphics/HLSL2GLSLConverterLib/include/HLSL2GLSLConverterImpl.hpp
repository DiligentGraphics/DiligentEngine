/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#pragma once

#include <list>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <array>

#include "HLSL2GLSLConverter.h"
#include "ObjectBase.hpp"
#include "HLSLKeywords.h"
#include "Shader.h"
#include "HashUtils.hpp"
#include "HLSLKeywords.h"
#include "Constants.h"

namespace Diligent
{

struct FunctionStubHashKey
{
    // clang-format off
    FunctionStubHashKey(const String& _Obj,  const String& _Func, Uint32 _NumArgs) : 
        Object      {_Obj    },
        Function    {_Func   },
        NumArguments{_NumArgs}
    {
    }

    FunctionStubHashKey(const Char* _Obj, const Char* _Func, Uint32 _NumArgs) : 
        Object      {_Obj    },
        Function    {_Func   },
        NumArguments{_NumArgs}
    {
    }

    FunctionStubHashKey(FunctionStubHashKey&& Key) :
        Object      {std::move(Key.Object)  },
        Function    {std::move(Key.Function)},
        NumArguments{Key.NumArguments       }
    {
    }
    // clang-format on

    bool operator==(const FunctionStubHashKey& rhs) const
    {
        return Object == rhs.Object &&
            Function == rhs.Function &&
            NumArguments == rhs.NumArguments;
    }

    HashMapStringKey Object;
    HashMapStringKey Function;
    Uint32           NumArguments;

    struct Hasher
    {
        size_t operator()(const FunctionStubHashKey& Key) const
        {
            return ComputeHash(Key.Object.GetHash(), Key.Function.GetHash(), Key.NumArguments);
        }
    };
};

/// HLSL to GLSL shader source code converter implementation
class HLSL2GLSLConverterImpl
{
public:
    static const HLSL2GLSLConverterImpl& GetInstance();

    // clang-format off

    /// Conversion attributes
    struct ConversionAttribs
    {
        /// Shader source input stream factory. Used to load shader source when HLSLSource is null as
        /// well as to load shader includes.
        IShaderSourceInputStreamFactory*    pSourceStreamFactory       = nullptr;

        /// Optional pointer to a conversion stream.
        IHLSL2GLSLConversionStream**        ppConversionStream         = nullptr;

        /// HLSL source code. Can be null, in which case the source code will be loaded from the
        /// input stream factory (that must not be null in this case) using InputFileName.
        const Char*                         HLSLSource                 = nullptr;

        /// Number of symbols in HLSLSource string. Ignored if HLSLSource is null.
        size_t                              NumSymbols                 = 0;

        /// Shader entry point.
        const Char*                         EntryPoint                 = nullptr;

        /// Shader type. See Diligent::SHADER_TYPE.
        SHADER_TYPE                         ShaderType                 = SHADER_TYPE_UNKNOWN;

        /// Whether to include GLSL definitions supporting HLSL->GLSL conversion.
        bool                                IncludeDefinitions         = false;

        /// Input file name. If HLSLSource is not null, this name will only be used for
        /// information purposes. If HLSLSource is null, the name will be used to load
        /// shader source from the input stream factory.
        const Char*                         InputFileName              = nullptr;

        /// Combined texture sampler suffix.
        const Char*                         SamplerSuffix              = "_sampler";

        /// Whether to use in-out location qualifiers. 
        /// This requires separate shader objects extension:
        /// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_separate_shader_objects.txt
        bool                                UseInOutLocationQualifiers = true;
    };

    // clang-format on

    /// Converts HLSL source to GLSL

    /// \param [in] Attribs - Conversion attributes.
    /// \return     Converted GLSL source code.
    String Convert(ConversionAttribs& Attribs) const;

    /// Creates a conversion stream

    /// \param [in] InputFileName - Input file name. If HLSLSource is null, this name will be
    ///                             used to load shader source code from the input stream factory.
    ///                             If HLSLSource is not null, the name will only be used for information
    ///                             purposes.
    /// \param [in] pSourceStreamFactory - Input stream factory that is used to load shader includes as well
    ///                                    as to load the shader source code if HLSLSource is null.
    /// \param [in] HLSLSource    - HLSL source code. If this parameter is null, the source will be loaded from
    ///                             the input stream factory using InputFileName.
    /// \param [in] NumSymbols    - Number of symbols in the HLSLSource string
    /// \prarm [out] ppStream     - Memory address where pointer to the created stream will be written
    void CreateStream(const Char*                      InputFileName,
                      IShaderSourceInputStreamFactory* pSourceStreamFactory,
                      const Char*                      HLSLSource,
                      size_t                           NumSymbols,
                      IHLSL2GLSLConversionStream**     ppStream) const;

private:
    HLSL2GLSLConverterImpl();

    struct HLSLObjectInfo
    {
        String GLSLType;      // sampler2D, sampler2DShadow, image2D, etc.
        Uint32 NumComponents; // 0,1,2,3 or 4
                              // Texture2D<float4>  -> 4
                              // Texture2D<uint>    -> 1
                              // Texture2D          -> 0
        HLSLObjectInfo(const String& Type, Uint32 NComp) :
            GLSLType{Type},
            NumComponents{NComp}
        {}
    };
    struct ObjectsTypeHashType
    {
        // This is only required to make the code compile on paranoid MSVC 2017 compiler (19.10.25017):
        // https://stackoverflow.com/questions/47604029/move-constructors-of-stl-containers-in-msvc-2017-are-not-marked-as-noexcept
        ObjectsTypeHashType() noexcept {}
        ObjectsTypeHashType(ObjectsTypeHashType&& rhs) noexcept :
            m{std::move(rhs.m)}
        {}
        // clang-format off
        ObjectsTypeHashType& operator = (ObjectsTypeHashType&&) = delete;
        ObjectsTypeHashType             (ObjectsTypeHashType&)  = delete;
        ObjectsTypeHashType& operator = (ObjectsTypeHashType&)  = delete;
        // clang-format on

        std::unordered_map<HashMapStringKey, HLSLObjectInfo, HashMapStringKey::Hasher> m;
    };

    struct GLSLStubInfo
    {
        String Name;
        String Swizzle;
        GLSLStubInfo(const String& _Name, const char* _Swizzle) :
            Name{_Name},
            Swizzle{_Swizzle}
        {}
    };
    // Hash map that maps GLSL object, method and number of arguments
    // passed to the original function, to the GLSL stub function
    // Example: {"sampler2D", "Sample", 2} -> {"Sample_2", "_SWIZZLE"}
    std::unordered_map<FunctionStubHashKey, GLSLStubInfo, FunctionStubHashKey::Hasher> m_GLSLStubs;

    // clang-format off
    enum class TokenType
    {
        Undefined,
#define ADD_KEYWORD(keyword)kw_##keyword,
        ITERATE_KEYWORDS(ADD_KEYWORD)
#undef ADD_KEYWORD
        PreprocessorDirective,
        Operator,
        OpenBrace,
        ClosingBrace,
        OpenBracket,
        ClosingBracket,
        OpenStaple,
        ClosingStaple,
        OpenAngleBracket,
        ClosingAngleBracket,
        Identifier,
        NumericConstant,
        SrtingConstant,
        Semicolon,
        Comma,
        TextBlock,
        Assignment,
        ComparisonOp,
        BooleanOp,
        BitwiseOp,
        IncDecOp,
        MathOp
    };
    // clang-format on

    struct TokenInfo
    {
        TokenType Type;
        String    Literal;
        String    Delimiter;

        bool IsBuiltInType() const
        {
            static_assert(static_cast<int>(TokenType::kw_bool) == 1 && static_cast<int>(TokenType::kw_void) == 191,
                          "If you updated built-in types, double check that all types are defined between bool and void");
            return Type >= TokenType::kw_bool && Type <= TokenType::kw_void;
        }

        bool IsFlowControl() const
        {
            static_assert(static_cast<int>(TokenType::kw_break) == 192 && static_cast<int>(TokenType::kw_while) == 202,
                          "If you updated control flow keywords, double check that all keywords are defined between break and while");
            return Type >= TokenType::kw_break && Type <= TokenType::kw_while;
        }

        TokenInfo(TokenType   _Type      = TokenType::Undefined,
                  const Char* _Literal   = "",
                  const Char* _Delimiter = "") :
            Type{_Type},
            Literal{_Literal},
            Delimiter{_Delimiter}
        {}
    };
    typedef std::list<TokenInfo> TokenListType;


    class ConversionStream : public ObjectBase<IHLSL2GLSLConversionStream>
    {
    public:
        using TBase = ObjectBase<IHLSL2GLSLConversionStream>;

        /// Conversion stream constructor.

        /// \param [in] pRefCounters  - Pointer to a reference counters object
        /// \param [in] Converter     - Reference to HLSL2GLSLConverterImpl class instance
        /// \param [in] InputFileName - Input file name. If HLSLSource is null, this name will be
        ///                             used to load shader source code from the input stream factory.
        ///                             If HLSLSource is not null, the name will only be used for information
        ///                             purposes.
        /// \param [in] pInputStreamFactory - Input stream factory that is used to load shader includes as well
        ///                                   as to load the shader source code if HLSLSource is null.
        /// \param [in] HLSLSource    - HLSL source code. If this parameter is null, the source will be loaded from
        ///                             the input stream factory using InputFileName.
        /// \param [in] NumSymbols    - Number of symbols in the HLSLSource string
        /// \param [in] bPreserveTokens - Whether to preserve original tokens. This must be set to true if the stream
        ///                               will be used for multiple conversions.
        ConversionStream(IReferenceCounters*              pRefCounters,
                         const HLSL2GLSLConverterImpl&    Converter,
                         const char*                      InputFileName,
                         IShaderSourceInputStreamFactory* pInputStreamFactory,
                         const Char*                      HLSLSource,
                         size_t                           NumSymbols,
                         bool                             bPreserveTokens);

        String Convert(const Char* EntryPoint,
                       SHADER_TYPE ShaderType,
                       bool        IncludeDefintions,
                       const char* SamplerSuffix,
                       bool        UseInOutLocationQualifiers);

        virtual void DILIGENT_CALL_TYPE Convert(const Char* EntryPoint,
                                                SHADER_TYPE ShaderType,
                                                bool        IncludeDefintions,
                                                const char* SamplerSuffix,
                                                bool        UseInOutLocationQualifiers,
                                                IDataBlob** ppGLSLSource) override final;

        IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_HLSL2GLSLConversionStream, TBase)

        const String& GetInputFileName() const { return m_InputFileName; }

    private:
        void InsertIncludes(String& GLSLSource, IShaderSourceInputStreamFactory* pSourceStreamFactory);
        void Tokenize(const String& Source);

        typedef std::unordered_map<String, bool> SamplerHashType;

        const HLSLObjectInfo* FindHLSLObject(const String& Name);

        void ProcessShaderDeclaration(TokenListType::iterator EntryPointToken, SHADER_TYPE ShaderType);

        void ProcessObjectMethods(const TokenListType::iterator& ScopeStart, const TokenListType::iterator& ScopeEnd);

        void ProcessRWTextures(const TokenListType::iterator& ScopeStart, const TokenListType::iterator& ScopeEnd);

        void ProcessAtomics(const TokenListType::iterator& ScopeStart,
                            const TokenListType::iterator& ScopeEnd);

        void RegisterStruct(TokenListType::iterator& Token);

        void ProcessConstantBuffer(TokenListType::iterator& Token);
        void ProcessStructuredBuffer(TokenListType::iterator& Token, Uint32& ShaderStorageBlockBinding);
        void ParseSamplers(TokenListType::iterator& ScopeStart, SamplerHashType& SamplersHash);

        void ProcessTextureDeclaration(TokenListType::iterator&            Token,
                                       const std::vector<SamplerHashType>& SamplersHash,
                                       ObjectsTypeHashType&                Objects,
                                       const char*                         SamplerSuffix,
                                       Uint32&                             ImageBinding);

        bool ProcessObjectMethod(TokenListType::iterator&       Token,
                                 const TokenListType::iterator& ScopeStart,
                                 const TokenListType::iterator& ScopeEnd);

        Uint32 CountFunctionArguments(TokenListType::iterator& Token, const TokenListType::iterator& ScopeEnd);
        bool   ProcessRWTextureStore(TokenListType::iterator& Token, const TokenListType::iterator& ScopeEnd);
        void   RemoveFlowControlAttribute(TokenListType::iterator& Token);
        void   RemoveSemantics();
        void   RemoveSpecialShaderAttributes();
        void   RemoveSemanticsFromBlock(TokenListType::iterator& Token, TokenType OpenBracketType, TokenType ClosingBracketType);
        void   RemoveSamplerRegister(TokenListType::iterator& Token);

        // IteratorType may be String::iterator or String::const_iterator.
        // While iterator is convertible to const_iterator,
        // iterator& cannot be converted to const_iterator& (Microsoft compiler allows
        // such conversion, while gcc does not)
        template <typename IteratorType>
        String PrintTokenContext(IteratorType& TargetToken, Int32 NumAdjacentLines);

        struct ShaderParameterInfo
        {
            enum class StorageQualifier : Int8
            {
                Unknown = 0,
                In      = 1,
                Out     = 2,
                InOut   = 3,
                Ret     = 4
            } storageQualifier;

            struct GSAttributes
            {
                enum class PrimitiveType : Int8
                {
                    Undefined   = 0,
                    Point       = 1,
                    Line        = 2,
                    Triangle    = 3,
                    LineAdj     = 4,
                    TriangleAdj = 5
                };
                enum class StreamType : Int8
                {
                    Undefined = 0,
                    Point     = 1,
                    Line      = 2,
                    Triangle  = 3
                };
                PrimitiveType PrimType;
                StreamType    Stream;
                GSAttributes() :
                    PrimType{PrimitiveType::Undefined},
                    Stream{StreamType::Undefined}
                {}
            } GSAttribs;

            struct HSAttributes
            {
                enum class InOutPatchType : Int8
                {
                    Undefined   = 0,
                    InputPatch  = 1,
                    OutputPatch = 2
                } PatchType;
                HSAttributes() :
                    PatchType{InOutPatchType::Undefined}
                {}
            } HSAttribs;

            String ArraySize;
            String Type;
            String Name;
            String Semantic;

            std::vector<ShaderParameterInfo> members;

            ShaderParameterInfo() :
                storageQualifier{StorageQualifier::Unknown}
            {}
        };
        void ParseShaderParameter(TokenListType::iterator& Token,
                                  ShaderParameterInfo&     ParamInfo);
        void ProcessFunctionParameters(TokenListType::iterator&          Token,
                                       std::vector<ShaderParameterInfo>& Params,
                                       bool&                             bIsVoid);
        bool RequiresFlatQualifier(const String& Type);
        void ProcessFragmentShaderArguments(std::vector<ShaderParameterInfo>& Params,
                                            String&                           GlobalVariables,
                                            std::stringstream&                ReturnHandlerSS,
                                            String&                           Prologue);

        String BuildParameterName(const std::vector<const ShaderParameterInfo*>& MemberStack,
                                  Char                                           Separator,
                                  const Char*                                    Prefix             = "",
                                  const Char*                                    SubstituteInstName = "",
                                  const Char*                                    Index              = "");

        template <typename THandler>
        void ProcessScope(TokenListType::iterator& Token,
                          TokenListType::iterator  ScopeEnd,
                          TokenType                OpenParenType,
                          TokenType                ClosingParenType,
                          THandler                 Handler);

        template <typename TArgHandler>
        void ProcessShaderArgument(const ShaderParameterInfo& Param,
                                   int                        ShaderInd,
                                   int                        IsOutVar,
                                   std::stringstream&         PrologueSS,
                                   TArgHandler                ArgHandler);

        void ProcessVertexShaderArguments(std::vector<ShaderParameterInfo>& Params,
                                          String&                           Globals,
                                          std::stringstream&                ReturnHandlerSS,
                                          String&                           Prologue);

        void ProcessGeometryShaderArguments(TokenListType::iterator&          TypeToken,
                                            std::vector<ShaderParameterInfo>& Params,
                                            String&                           Globals,
                                            String&                           Prologue);

        void ProcessHullShaderConstantFunction(const Char* FuncName, bool& bTakesInputPatch);

        void ProcessShaderAttributes(TokenListType::iterator&                                                TypeToken,
                                     std::unordered_map<HashMapStringKey, String, HashMapStringKey::Hasher>& Attributes);

        void ProcessHullShaderArguments(TokenListType::iterator&          TypeToken,
                                        std::vector<ShaderParameterInfo>& Params,
                                        String&                           Globals,
                                        std::stringstream&                ReturnHandlerSS,
                                        String&                           Prologue);
        void ProcessDomainShaderArguments(TokenListType::iterator&          TypeToken,
                                          std::vector<ShaderParameterInfo>& Params,
                                          String&                           Globals,
                                          std::stringstream&                ReturnHandlerSS,
                                          String&                           Prologue);
        void ProcessComputeShaderArguments(TokenListType::iterator&          TypeToken,
                                           std::vector<ShaderParameterInfo>& Params,
                                           String&                           Globals,
                                           String&                           Prologue);

        void FindClosingBracket(TokenListType::iterator&       Token,
                                const TokenListType::iterator& ScopeEnd,
                                TokenType                      OpenBracketType,
                                TokenType                      ClosingBracketType);

        void ProcessReturnStatements(TokenListType::iterator& Token,
                                     bool                     IsVoid,
                                     const Char*              EntryPoint,
                                     const Char*              MacroName);

        void ProcessGSOutStreamOperations(TokenListType::iterator& Token,
                                          const String&            OutStreamName,
                                          const char*              EntryPoint);

        String BuildGLSLSource();

        // Tokenized source code
        TokenListType m_Tokens;

        // List of tokens defining structs
        std::unordered_map<HashMapStringKey, TokenListType::iterator, HashMapStringKey::Hasher> m_StructDefinitions;

        // Stack of parsed objects, for every scope level.
        // There are currently only two levels:
        // level 0 - global scope, contains all global objects
        //           (textures, buffers)
        // level 1 - function body, contains all objects
        //           defined as function arguments
        std::vector<ObjectsTypeHashType> m_Objects;

        const bool m_bPreserveTokens;
        bool       m_bUseInOutLocationQualifiers = true;

        const HLSL2GLSLConverterImpl& m_Converter;

        // This member is only used to compare input name
        // when subsequent shaders are converted from already tokenized source
        const String m_InputFileName;
    };

    // HLSL keyword->token info hash map
    // Example: "Texture2D" -> TokenInfo(TokenType::Texture2D, "Texture2D")
    std::unordered_map<HashMapStringKey, TokenInfo, HashMapStringKey::Hasher> m_HLSLKeywords;

    // Set of all GLSL image types (image1D, uimage1D, iimage1D, image2D, ... )
    std::unordered_set<HashMapStringKey, HashMapStringKey::Hasher> m_ImageTypes;

    // Set of all HLSL atomic operations (InterlockedAdd, InterlockedOr, ...)
    std::unordered_set<HashMapStringKey, HashMapStringKey::Hasher> m_AtomicOperations;

    // HLSL semantic -> glsl variable, for every shader stage and input/output type (in == 0, out == 1)
    // Example: [vertex, output] SV_Position -> gl_Position
    //          [fragment, input] SV_Position -> gl_FragCoord
    static constexpr int InVar           = 0;
    static constexpr int OutVar          = 1;
    static constexpr int MaxShaderStages = 6; // Maximum supported shader stages: VS, GS, PS, DS, HS, CS

    std::array<std::array<std::unordered_map<HashMapStringKey, String, HashMapStringKey::Hasher>, 2>, MaxShaderStages> m_HLSLSemanticToGLSLVar;
};

} // namespace Diligent

//  Intro
// DirectX and OpenGL use different shading languages. While mostly being very similar,
// the language syntax differs substantially sometimes. Having two versions of each
// shader is clearly not an option for real projects. Maintaining intermediate representation
// that translates to both languages is one solution, but it might complicate shader development
// and debugging.
//
// HLSL converter allows HLSL shader files to be converted into GLSL source.
// The entire shader development can thus be performed using HLSL tools. Since no intermediate
// representation is used, shader files can be directly compiled by the HLSL compiler.
// All tools available for HLSL shader devlopment, analysis and optimization can be
// used. The source can then be transaprently converted to GLSL.
//
//
//  Using HLSL Converter
// * The following rules are used to convert HLSL texture declaration into GLSL sampler:
//   - HLSL texture dimension defines GLSL sampler dimension:
//        - Texture2D   -> sampler2D
//        - TextureCube -> samplerCube
//   - HLSL texture component type defines GLSL sampler type. If no type is specified, float4 is assumed:
//        - Texture2D<float>     -> sampler2D
//        - Texture3D<uint4>     -> usampler3D
//        - Texture2DArray<int2> -> isampler2DArray
//        - Texture2D            -> sampler2D
//    - To distinguish if sampler should be shadow or not, the converter tries to find <Texture Name>_sampler
//      among samplers (global variables and function arguments). If the sampler type is comparison,
//      the texture is converted to shadow sampler. If sampler state is either not comparison or not found,
//      regular sampler is used.
//      Examples:
//        - Texture2D g_ShadowMap;                        -> sampler2DShadow g_ShadowMap;
//          SamplerComparisonState g_ShadowMap_sampler;
//        - Texture2D g_Tex2D;                            -> sampler2D g_Tex2D;
//          SamplerState g_Tex2D_sampler;
//          Texture3D g_Tex3D;                            -> sampler3D g_Tex3D;
//
// * GLSL requires format to be specified for all images allowing writes. HLSL converter allows GLSL image
//   format specification inside the special comment block:
//   Example:
//     RWTexture2D<float /* format=r32f */ > Tex2D;
// * In OpenGL tessellation, domain, partitioning, and topology are properties of tessellation evaluation
//   shader rather than tessellation control shader. The following specially formatted comment should be placed
//   on top of domain shader declararion to specify the attributes
//       /* partitioning = {integer|fractional_even|fractional_odd}, outputtopology = {triangle_cw|triangle_ccw} */
//   Example:
//       /* partitioning = fractional_even, outputtopology = triangle_cw */


//  Requirements:
//  * GLSL allows samplers to be declared as global variables or function arguments only.
//    It does not allow local variables of sampler type.
//
// Important notes/known issues:
//
// * GLSL compiler does not handle float3 structure members correctly. It is
//   strongly suggested not to use this type in structure definitions
//
// * At least NVidia GLSL compiler does not apply layout(row_major) to
//   structure members. By default, all matrices in both HLSL and GLSL
//   are column major
//
// * GLSL compiler does not properly handle structs passed as function arguments!!!!
//   struct MyStruct
//   {
//        matrix Matr;
//   }
//   void Func(in MyStruct S)
//   {
//        ...
//        mul(f4PosWS, S.Matr); <--- This will not work!!!
//   }
//   DO NOT pass structs to functions, use only built-in types!!!
//
// * GLSL does not support most of the implicit type conversions. The following are some
//   examples of required modifications to HLSL code:
//   ** float4 vec = 0; ->  float4 vec = float4(0.0, 0.0, 0.0, 0.0);
//   ** float x = 0;    ->  float x = 0.0;
//   ** uint x = 0;     ->  uint x = 0u;
//   ** GLES is immensely strict about type conversions. For instance,
//      this code will produce compiler error: float4(0, 0, 0, 0)
//      It must be written as float4(0.0, 0.0, 0.0, 0.0)
// * GLSL does not support relational and boolean operations on vector types:
//   ** float2 p = float2(1.0,2.0), q = float2(3.0,4.0);
//      bool2 b = x < y;   -> Error
//      all(p<q)           -> Error
//   ** To facilitate relational and boolean operations on vector types, the following
//      functions are predefined:
//      - Less
//      - LessEqual
//      - Greater
//      - GreaterEqual
//      - Equal
//      - NotEqual
//      - Not
//      - And
//      - Or
//      - BoolToFloat
//   ** Examples:
//      bool2 b = x < y;   -> bool2 b = Less(x, y);
//      all(p>=q)          -> all( GreaterEqual(p,q) )
//
// * When accessing elements of an HLSL matrix, the first index is always row:
//     mat[row][column]
//   In GLSL, the first index is always column:
//     mat[column][row]
//   MATRIX_ELEMENT(mat, row, col) macros is provided to facilitate matrix element retrieval

// * The following functions do not have counterparts in GLSL and should be avoided:
//   ** Texture2DArray.SampleCmpLevelZero()
//   ** TextureCube.SampleCmpLevelZero()
//   ** TextureCubeArray.SampleCmpLevelZero()


// * Input variables to a shader stage must be listed in exact same order as outputs from
//   the previous stage. Function return value counts as the first output argument.
