Shader " Vertex Colored" 
{
	Properties 
	{
		_Color ("Diffuse Color", Color) = (1,1,1,1)
		_MainTex ("Texture", 2D) = "white" {}
	}
	
	SubShader 
	{
		Tags { "RenderType" = "Opaque" }
		CGPROGRAM
		#pragma surface surf Lambert
		struct Input 
		{
			float2 uv_MainTex;
			float4 color: Color; // Vertex color
		};
		sampler2D _MainTex;
		fixed4 _Color;
		void surf (Input IN, inout SurfaceOutput o) 
		{
			o.Albedo = tex2D (_MainTex, IN.uv_MainTex).rgb;
			o.Albedo *= _Color;
			o.Albedo *= IN.color.rgb;
		}
		ENDCG
	}
	
	Fallback "Diffuse", 1
}