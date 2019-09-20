/*
 * ================= AnimFrameSet.cpp ==========================
 *                          -- tpr --
 *                                        CREATE -- 2018.11.23
 *                                        MODIFY -- 
 * ----------------------------------------------------------
 */
#include "AnimFrameSet.h"

//-------------------- CPP --------------------//
#include <algorithm> //- find
#include <iterator>
#include <utility> //- move

//------------------- Libs --------------------//
#include "tprGeneral.h" 

//------------------- Engine --------------------//
#include "tprAssert.h"
#include "global.h"
#include "Pjt_RGBAHandle.h"
#include "create_texNames.h"
#include "load_and_divide_png.h"

#include "esrc_animFrameSet.h"

#include "tprCast.h"

#include "tprDebug.h" //- tmp


namespace afs_inn {//----------------- namespace: afs_inn ------------------//

    //-- 每次 insert png数据时，以下这些数据都会被复用 --
    size_t headIdx {}; //- 本次 png数据insert，起始idx
                       //- 在此之前，容器中以及有数据了。

    //-- 图元帧 数据容器组。帧排序为 [left-top] --
    std::vector<std::vector<RGBA>> P_frame_data_ary {}; 
    std::vector<std::vector<RGBA>> J_frame_data_ary {}; 
    std::vector<std::vector<RGBA>> S_frame_data_ary {};

    //- 画面 贴图集的 相对路径名。一个动作的所有帧图片，存储为一张 png图。
    //- 这个 路径名 只在游戏启动阶段被使用，之后 预存于此
    //- local path，based on path_animFrameSet_srcs
    std::string  lpath_pic    {}; //-- picture
    std::string  lpath_pjt    {}; //-- collients
    std::string  lpath_shadow {}; //-- shadow  (此文件也许可为空...)

    IntVec2  pixNum_per_frame {};  //- 单帧画面 的 长宽 像素值 （会被存到 animAction 实例中）
    IntVec2  frameNum         {};  //- 画面中，横排可分为几帧，纵向可分为几帧
    size_t   totalFrameNum    {};  //- 目标png文件中，总 图元帧 个数
    ColliderType  colliderType {}; //- 碰撞体类型: nil / circular / capsule

    std::vector<GLuint> tmpTexNames {}; //- 用于 create_texNames()

    std::unordered_map<size_t, animActionPosId_t> lAnimActionPosIds {}; //- key: animActionParams_.endIdx

    //===== funcs =====//
    void build_three_lpaths( const std::string &lpath_pic_ );


}//-------------------- namespace: afs_inn end ------------------//


/* ===========================================================
 *                  insert_a_png
 * -----------------------------------------------------------
 */
void AnimFrameSet::insert_a_png(  const std::string &lpath_pic_, 
                            IntVec2             frameNum_,
                            size_t              totalFrameNum_,
                            bool                isHaveShadow_, //-- 将被记录到 animAction 数据中去
                            bool                isPjtSingleFrame_,
                            bool                isShadowSingleFrame_,
                            ColliderType        colliderType_,
                            const std::vector<std::shared_ptr<AnimActionParam>> &animActionParams_ ){

    afs_inn::build_three_lpaths( lpath_pic_ );
    afs_inn::frameNum = frameNum_;
    afs_inn::totalFrameNum = totalFrameNum_;
    afs_inn::colliderType = colliderType_;

    this->isPJTSingleFrame = isPjtSingleFrame_;
    this->isShadowSingleFrame = isShadowSingleFrame_;
    
    //-------------------//
    //-- 获得本次 insert 的 起始idx
    afs_inn::headIdx = this->texNames_pic.size();

    //----------------------------------------//
    //          lAnimActionPosUPtrs
    //----------------------------------------//
    afs_inn::lAnimActionPosIds.clear();
    if( this->isPJTSingleFrame ){
        //--- 所有 actionName,绑定同一个 id ---
        auto aaPosId = AnimActionPos::id_manager.apply_a_u32_id();
        this->animActionPosUPtrs.insert({ aaPosId, std::make_unique<AnimActionPos>() });
        //---
        for( size_t i=0; i<animActionParams_.size(); i++ ){
            const AnimActionParam *paramPtr = animActionParams_.at(i).get();

            tprAssert( afs_inn::lAnimActionPosIds.find(i) == afs_inn::lAnimActionPosIds.end() );
            afs_inn::lAnimActionPosIds.insert({ i, aaPosId });
        }

    }else{
        //--- 每个 actionName, 绑定各自的 id ---

        for( size_t i=0; i<animActionParams_.size(); i++ ){
            const AnimActionParam *paramPtr = animActionParams_.at(i).get();
            //---
            auto aaPosId = AnimActionPos::id_manager.apply_a_u32_id();
            this->animActionPosUPtrs.insert({ aaPosId, std::make_unique<AnimActionPos>() });
            //---
            tprAssert( afs_inn::lAnimActionPosIds.find(i) == afs_inn::lAnimActionPosIds.end() );
            afs_inn::lAnimActionPosIds.insert({ i, aaPosId });
        }
    }


    //----------------------------------------//
    //  load & divide png数据，存入每个 帧容器中
    //----------------------------------------//
    afs_inn::P_frame_data_ary.clear();
    afs_inn::J_frame_data_ary.clear();
    afs_inn::S_frame_data_ary.clear();

    IntVec2 tmpv2 {}; //-tmp

    //-------------------//
    //       Pic
    //-------------------//
    //-- 目前暂不支持 没有 pic数据的 AnimFrameSet 实例 ---
    afs_inn::pixNum_per_frame = load_and_divide_png( tprGeneral::path_combine( path_animFrameSets, afs_inn::lpath_pic ),
                                            afs_inn::frameNum,
                                            afs_inn::totalFrameNum,
                                            afs_inn::P_frame_data_ary );

    afs_inn::tmpTexNames.clear();
    create_texNames( afs_inn::totalFrameNum,
                     afs_inn::pixNum_per_frame,
                     afs_inn::P_frame_data_ary,
                     afs_inn::tmpTexNames );
    this->texNames_pic.insert( this->texNames_pic.end(), afs_inn::tmpTexNames.begin(), afs_inn::tmpTexNames.end() ); //- copy

    //-------------------//
    //       Pjt
    //-------------------//
    if( this->isPJTSingleFrame ){
        //--- J.png 只有一帧 --//
        tmpv2 = load_and_divide_png( tprGeneral::path_combine( path_animFrameSets, afs_inn::lpath_pjt ),
                                    IntVec2{1,1},
                                    1,
                                    afs_inn::J_frame_data_ary );

    }else{
        tmpv2 = load_and_divide_png( tprGeneral::path_combine( path_animFrameSets, afs_inn::lpath_pjt ),
                                    afs_inn::frameNum,
                                    afs_inn::totalFrameNum,
                                    afs_inn::J_frame_data_ary );
    }
    tprAssert( tmpv2 == afs_inn::pixNum_per_frame );
    this->handle_pjt( animActionParams_ );

    //-------------------//
    //      Shadow
    //-------------------//
    if( isHaveShadow_ ){
        if( this->isShadowSingleFrame ){
            //--- S.png 只有一帧 --//
            tmpv2 = load_and_divide_png( tprGeneral::path_combine( path_animFrameSets, afs_inn::lpath_shadow ),
                                        IntVec2{1,1},
                                        1,
                                        afs_inn::S_frame_data_ary );

        }else{
            tmpv2 = load_and_divide_png( tprGeneral::path_combine( path_animFrameSets, afs_inn::lpath_shadow ),
                                        afs_inn::frameNum,
                                        afs_inn::totalFrameNum,
                                        afs_inn::S_frame_data_ary );
        }

        tprAssert( tmpv2 == afs_inn::pixNum_per_frame );
        this->handle_shadow();
        afs_inn::tmpTexNames.clear();
        create_texNames( afs_inn::totalFrameNum,
                        afs_inn::pixNum_per_frame,
                        afs_inn::S_frame_data_ary,
                        afs_inn::tmpTexNames );
        this->texNames_shadow.insert( this->texNames_shadow.end(), afs_inn::tmpTexNames.begin(), afs_inn::tmpTexNames.end() );
        
    }else{
        //-- 没有 shadow数据时，填写 0 来占位 --
        this->texNames_shadow.insert(   this->texNames_shadow.end(), 
                                        afs_inn::totalFrameNum, 
                                        0 );
    }

    //-------------------//
    //    animActions
    //-------------------//

    for( size_t i=0; i<animActionParams_.size(); i++ ){
        const AnimActionParam *paramPtr = animActionParams_.at(i).get();
        //---
        animSubspeciesId_t subId = this->subGroup.find_or_insert_a_animSubspeciesId(paramPtr->animLabels,
                                                                                    paramPtr->subspeciesIdx );

        AnimSubspecies &subRef = esrc::find_or_insert_new_animSubspecies( subId );
        AnimAction &actionRef = subRef.insert_new_animAction( paramPtr->actionName );


        auto aaposId = afs_inn::lAnimActionPosIds.at( i );

        actionRef.init( *this,
                        *paramPtr,
                        this->animActionPosUPtrs.at(aaposId).get(),
                        afs_inn::pixNum_per_frame,
                        afs_inn::headIdx,
                        isHaveShadow_ );

    }
    this->subGroup.check(); //- MUST !!!    
}



/* ===========================================================
 *                   handle_pjt
 * -----------------------------------------------------------
 */
void AnimFrameSet::handle_pjt( const std::vector<std::shared_ptr<AnimActionParam>> &animActionParams_ ){

    size_t pixNum = cast_2_size_t( afs_inn::pixNum_per_frame.x * 
                                    afs_inn::pixNum_per_frame.y); //- 一帧有几个像素点
    Pjt_RGBAHandle  jh2 {5};
    glm::dvec2  pixDPos {}; //- current pix dpos

    if( this->isPJTSingleFrame ){
        //-- 此时，所有 actionName 都指向同一个 id --
        auto aaposId = afs_inn::lAnimActionPosIds.begin()->second;

        AnimActionSemiData frameSemiData {afs_inn::colliderType}; // MUST create NewOne !!!

        for( size_t p=0; p<pixNum; p++ ){ //--- each frame.pix [left-bottom]

            pixDPos.x = static_cast<double>( static_cast<int>(p)%afs_inn::pixNum_per_frame.x );
            pixDPos.y = static_cast<double>( static_cast<int>(p)/afs_inn::pixNum_per_frame.x );

            jh2.set_rgba(   &frameSemiData,
                            afs_inn::J_frame_data_ary.at(0).at(p), //- 只有一帧 
                            pixDPos );

        }//------ each frame.pix ------

        //--- 执行所有 补充性设置， IMPORTANT!!! --- 
        this->animActionPosUPtrs.at(aaposId)->init_from_semiData( frameSemiData );

    }else{

        for( size_t i=0; i<animActionParams_.size(); i++ ){
            const AnimActionParam *paramPtr = animActionParams_.at(i).get();
            //---
            auto aaposId = afs_inn::lAnimActionPosIds.at(i);
            AnimActionSemiData frameSemiData {afs_inn::colliderType}; // MUST create NewOne !!!

            for( size_t p=0; p<pixNum; p++ ){ //--- each frame.pix [left-bottom]

                pixDPos.x = static_cast<double>( static_cast<int>(p)%afs_inn::pixNum_per_frame.x );
                pixDPos.y = static_cast<double>( static_cast<int>(p)/afs_inn::pixNum_per_frame.x );

                jh2.set_rgba(   &frameSemiData,
                                afs_inn::J_frame_data_ary.at(paramPtr->jFrameIdx).at(p),
                                pixDPos );

            }//------ each frame.pix ------

            //--- 执行所有 补充性设置， IMPORTANT!!! --- 
            this->animActionPosUPtrs.at(aaposId)->init_from_semiData( frameSemiData );
        }
    }
}


/* ===========================================================
 *                   handle_shadow
 * -----------------------------------------------------------
 */
void AnimFrameSet::handle_shadow(){

    size_t pixNum = cast_2_size_t( afs_inn::pixNum_per_frame.x * 
                                    afs_inn::pixNum_per_frame.y); //- 一帧有几个像素点
    RGBA    color_shadow { 0,0,0, 255 };
    size_t  idx_for_S_frame_data_ary {};

    for( size_t f=0; f<afs_inn::totalFrameNum; f++ ){ //--- each frame ---
        for( size_t p=0; p<pixNum; p++ ){ //--- each frame.pix [left-bottom]

        //-- 通过此装置，实现了 “相同的 pjt数据 只用记录一帧” --
        //   并不是最优解，会做很多次重复运算，但是是对原有代码改动最少的。
        this->isShadowSingleFrame ?
                idx_for_S_frame_data_ary=0 :
                idx_for_S_frame_data_ary=f;

        RGBA &rgbaRef = afs_inn::S_frame_data_ary.at(idx_for_S_frame_data_ary).at(p);
        if( rgbaRef.is_near(color_shadow, 5) == false ){
            continue; //- next frame.pix
        }
        
        //-- 将 shadow pix 改为需要的颜色 --//

            // 一个临时的简陋的方案...
            // 在推荐方案中，为 shadow 专门配备一个 fs着色器，具体颜色在 着色器程序中调整

        rgbaRef.r = 0;
        rgbaRef.g = 5;
        rgbaRef.b = 10;
        rgbaRef.a = 70;
        }
    }
}


namespace afs_inn {//----------------- namespace: afs_inn ------------------//


/* ===========================================================
 *                 build_three_lpaths
 * -----------------------------------------------------------
 */
void build_three_lpaths( const std::string &lpath_pic_ ){
    //- 注释 以 lpath_pic = "/animal/dog_ack_01.P.png" 为例

    std::string lst {}; //- tmp, 尾部字符串，不停地被截断

    lpath_pic = lpath_pic_;

    //--------------------//
    //    生成 lpath_pjt
    //--------------------//
    auto point_idx = lpath_pic.find( '.', 0 ); //- 指向第一个 '.'
    auto lastIt = lpath_pic.begin();
    std::advance( lastIt, point_idx ); //- advance 并不防止 溢出
    //- lpath_pjt 暂时等于 "/animal/dog_ack_01"
    lpath_pjt.assign( lpath_pic.begin(), lastIt );
    lpath_pjt += ".J.png";

    //--------------------//
    //    生成 lpath_shadow
    //--------------------//
    //- lpath_shadow 暂时等于 "/animal/dog_ack_01"
    lpath_shadow.assign( lpath_pic.begin(), lastIt );
    lpath_shadow += ".S.png";
}

}//-------------------- namespace: afs_inn end ------------------//