#include "vrayalmtl.h"

#include "vraybase.h"
#include "vrayinterface.h"
#include "shadedata_new.h"
#include "tomax.h"
#include "vraycore.h"
#include "vrayrenderer.h"

/*===========================================================================*\
 |	Determine the characteristics of the material
\*===========================================================================*/

void SkeletonMaterial::SetAmbient(Color c, TimeValue t) {}		
void SkeletonMaterial::SetDiffuse(Color c, TimeValue t) {}		
void SkeletonMaterial::SetSpecular(Color c, TimeValue t) {}
void SkeletonMaterial::SetShininess(float v, TimeValue t) {}
				
Color SkeletonMaterial::GetAmbient(int mtlNum, BOOL backFace) {
	return Color(0,0,0);
}

Color SkeletonMaterial::GetDiffuse(int mtlNum, BOOL backFace) {
	return Color(0.5f, 0.5f, 0.5f);
}

Color SkeletonMaterial::GetSpecular(int mtlNum, BOOL backFace) {
	return Color(0,0,0);
}

float SkeletonMaterial::GetXParency(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float SkeletonMaterial::GetShininess(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float SkeletonMaterial::GetShinStr(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float SkeletonMaterial::WireSize(int mtlNum, BOOL backFace) {
	return 0.0f;
}

		
/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void SkeletonMaterial::Shade(ShadeContext &sc) {
	if (sc.ClassID()==VRAYCONTEXT_CLASS_ID)
		shade(static_cast<VR::VRayInterface&>(sc), gbufID);
	else {
		if (gbufID) sc.SetGBufferID(gbufID);
		sc.out.c.Black(); sc.out.t.Black();
	}
}

float SkeletonMaterial::EvalDisplacement(ShadeContext& sc)
{
	return 0.0f;
}

Interval SkeletonMaterial::DisplacementValidity(TimeValue t)
{
	Interval iv;
	iv.SetInfinite();
	return iv;
}
	
VR::Color SkeletonMaterial::combineTex(const VR::VRayContext &rc, const VR::Color &origColor, int texIndex) const {
	if (!subtex[texIndex]) return origColor;

	AColor texColor(0.0f, 0.0f, 0.0f);
	texColor=subtex[texIndex]->EvalColor((VR::VRayInterface&) rc);

	if (!(texColor.r>0.0f)) texColor.r=0.0f;
	if (!(texColor.g>0.0f)) texColor.g=0.0f;
	if (!(texColor.b>0.0f)) texColor.b=0.0f;

	float alpha=VR::clamp(texColor.a, 0.0f, 1.0f);

	float m=subtexMult[texIndex];
	VR::Color res=origColor*(1.0f-m*alpha)+VR::Color(texColor.r, texColor.g, texColor.b)*m;
	return res;
}

float SkeletonMaterial::combineTex(const VR::VRayContext &rc, float origValue, int texIndex) const {
	if (!subtex[texIndex]) return origValue;

	float texValue=subtex[texIndex]->EvalMono((VR::VRayInterface&) rc);
	float m=subtexMult[texIndex];
	
	float res=origValue*(1.0f-m)+texValue*m;
	return res;
}

VR::BSDFSampler* SkeletonMaterial::newBSDF(const VR::VRayContext &rc, VR::VRenderMtlFlags flags) {
	MyALBSDF *bsdf=bsdfPool.newBRDF(rc);
	if (!bsdf) return NULL;

	VR::ALBSDFParams &params=bsdf->getParams();

	params.diffuse=combineTex(rc, toColor(diffuse), SUBTEXNO_DIFFUSE);
	params.diffuse*=combineTex(rc, diffuseStrength, SUBTEXNO_DIFFUSE_STRENGTH);

	params.reflectColor1=combineTex(rc, toColor(reflectColor1), SUBTEXNO_REFLECT1_COLOR);
	params.reflectColor1*=combineTex(rc, reflectStrength1, SUBTEXNO_REFLECT1_STRENGTH);
	params.reflectRoughness1=combineTex(rc, reflectRoughness1, SUBTEXNO_REFLECT1_ROUGHNESS);
	params.reflectIOR1=combineTex(rc, reflectIOR1, SUBTEXNO_REFLECT1_IOR);
	
	params.transparency=combineTex(rc, toColor(opacity), SUBTEXNO_OPACITY).whiteComplement();
	params.subdivs=8;
	params.doubleSided=true;

	params.sssMix=combineTex(rc, sssMix, SUBTEXNO_SSS_MIX);

	params.sssWeight1=combineTex(rc, sssWeight1, SUBTEXNO_SSS1_WEIGHT);
	params.sssColor1=combineTex(rc, toColor(sssColor1), SUBTEXNO_SSS1_COLOR);
	params.sssRadius1=combineTex(rc, sssRadius1, SUBTEXNO_SSS1_RADIUS);

	params.sssWeight2=combineTex(rc, sssWeight2, SUBTEXNO_SSS2_WEIGHT);
	params.sssColor2=combineTex(rc, toColor(sssColor2), SUBTEXNO_SSS2_COLOR);
	params.sssRadius2=combineTex(rc, sssRadius2, SUBTEXNO_SSS2_RADIUS);

	params.sssWeight3=combineTex(rc, sssWeight3, SUBTEXNO_SSS3_WEIGHT);
	params.sssColor3=combineTex(rc, toColor(sssColor3), SUBTEXNO_SSS3_COLOR);
	params.sssRadius3=combineTex(rc, sssRadius3, SUBTEXNO_SSS3_RADIUS);

	params.sssDensityScale=sssDensityScale;

	params.normalizeWeights();

	bsdf->init(rc);
	return bsdf;
}

void SkeletonMaterial::deleteBSDF(const VR::VRayContext &rc, VR::BSDFSampler *b) {
	if (!b) return;
	MyALBSDF *bsdf=static_cast<MyALBSDF*>(b);
	bsdfPool.deleteBRDF(rc, bsdf);
}

VR::VRayVolume* SkeletonMaterial::getVolume(const VR::VRayContext &rc) {
	return NULL;
}

void SkeletonMaterial::addRenderChannel(int index) {
	renderChannels+=index;
}
